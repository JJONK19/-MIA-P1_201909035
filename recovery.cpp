#include "recovery.h"


void recovery(std::vector<std::string> &parametros, std::vector<disco> &discos){

    //VARIABLES
    bool paramFlag = true;                     //Indica si se cumplen con los parametros del comando
    bool required = true;                      //Indica si vienen los parametros obligatorios
    FILE *archivo;                             //Sirve para verificar que el archivo exista       
    std::string id = "";                       //Atributo id
    std::string diskName;                      //Nombre del disco
    int posDisco = -1;                         //Posicion del disco en el vector
    int posParticion = -1;                     //Posicion de la particion en la lista del disco
    int posInicio;                             //Posicion donde inicia la particion
    int posLectura;                            //Para determinar la posicion de lectura en disco
    inodo cinodo;                              //Nuevo Inodo
    bcarpetas ccarpeta;                        //Nueva carpeta
    barchivos carchivo;                       //Nueva carpeta
    sbloque sblock;                            //Para leer el superbloque
    std::vector<registro> journal;             //Vector con los registros del sistema
    char c = '\0';

    //COMPROBACIÓN DE PARAMETROS
    for(int i = 1; i < parametros.size(); i++){
        std::string &temp = parametros[i];
        std::vector<std::string> salida(std::sregex_token_iterator(temp.begin(), temp.end(), flecha, -1),
                    std::sregex_token_iterator());

        std::string &tag = salida[0];
        std::string &value = salida[1];

        //Pasar a minusculas
        transform(tag.begin(), tag.end(), tag.begin(),[](unsigned char c){
            return tolower(c);
        });

        if(tag == "id"){
            id = value;
        }else{
            std::cout << "ERROR: El parametro " << tag << " no es valido." << std::endl;
            paramFlag = false;
            break;
        }
    }

    if(!paramFlag){
        return;
    }

    //COMPROBAR PARAMETROS OBLIGATORIOS
    if(id == ""){
        required = false;
    }

    if(!required){
        std::cout << "ERROR: La instrucción recovery carece de todos los parametros obligatorios." << std::endl;
        return;
    }

    //REMOVER LOS NUMEROS DEL ID PARA OBTENER EL NOMBRE DEL DISCO
    int posicion = 0;
    for(int i = 0; i < id.length(); i++){
        if(isdigit(id[i])){
            posicion++;
        }else{
            break;
        }
    }
    diskName = id.substr(posicion, id.length()-1);

    //BUSCAR EL DISCO EN EL VECTOR
    for(int i = 0; i < discos.size(); i++){
        disco temp = discos[i];
        if(temp.nombre == diskName){
            posDisco = i;
            break;
        }
    }

    if(posDisco == -1){
        std::cout << "ERROR: El disco no está montado." << std::endl;
        return;
    }

    //BUSCAR LA PARTICION DENTRO DEL DISCO MONTADO
    disco &tempD = discos[posDisco];
    for(int i = 0; i < tempD.particiones.size(); i++){
        montada temp = tempD.particiones[i];
        if(temp.id == id){
            posParticion = i;
            break;
        }
    }

    if(posParticion == -1){
        std::cout << "ERROR: La particion que desea dañar no existe." << std::endl;
        return;
    }

    //REVISAR SI EXISTE EL ARCHIVO 
    montada &formatear = tempD.particiones[posParticion];
    archivo = fopen(tempD.ruta.c_str(), "r+b");

    if(archivo == NULL){
        std::cout << "ERROR: El disco fisico no existe." << std::endl;
        return;
    }

    //DETERMINAR LA POSICION DE INICIO DE LA PARTICION
    if(formatear.posMBR != -1){
        MBR mbr;
        fseek(archivo, 0,SEEK_SET);
        fread(&mbr, sizeof(MBR), 1, archivo);
        posInicio = mbr.mbr_partition[formatear.posMBR].part_start;
    }else{
        EBR ebr;
        fseek(archivo, formatear.posEBR, SEEK_SET);
        fread(&ebr, sizeof(EBR), 1, archivo);
        posInicio = ebr.part_start;
    }

    //LEER EL SUPERBLOQUE
    fseek(archivo, posInicio, SEEK_SET);
    fread(&sblock, sizeof(sbloque), 1, archivo);

    //VERIFICAR QUE ES EXT3
    if(sblock.s_filesystem_type != 3){
        std::cout << "ERROR: El sistema debe de ser EXT3 para acceder a esta funcion." << std::endl;
        fclose(archivo);
        return;
    }

    //LEER EL JOURNAL Y AÑADIR LAS INSTRUCCIONES AL VECTOR  
    //Se empieza desde el 2 porque el 0 y el 1 son la carpeta raiz y el archivo de usuarios
    for(int i = 2; i < sblock.s_inodes_count; i++){
        registro creacion;

        //Buscar los journal
        posLectura = posInicio + sizeof(sbloque) + (sizeof(registro) * i);
        fseek(archivo, posLectura, SEEK_SET);
        fread(&creacion, sizeof(registro), 1, archivo);

        if(creacion.comando[0] != '\0'){
            journal.push_back(creacion);
        }else{
            break;
        }
    }

    //REINICIAR EL JOURNAL
    int numero = (sblock.s_inodes_count - 2) * sizeof(registro);
    for(int a = 0; a < numero; a++){
        posLectura = posInicio + sizeof(sbloque) + (sizeof(registro) * 2) + (sizeof(char) * a);
        fseek(archivo, posLectura, SEEK_SET);
        fwrite(&c ,sizeof(char), 1 ,archivo);
    }

    //CREAR LA CARPETA RAIZ Y EL ARCHIVO DE USUARIOS
    //Marcar el primer inodo
    c = '1';
    fseek(archivo, sblock.s_bm_inode_start, SEEK_SET);
    fwrite(&c, sizeof(char), 1, archivo);

    //MARCAR EL PRIMER BLOQUE
    c = 'c';
    fseek(archivo, sblock.s_bm_block_start, SEEK_SET);
    fwrite(&c, sizeof(char), 1, archivo);

    //CREAR Y ESCRIBIR EL INODO
    cinodo.i_uid = 1;
    cinodo.i_gid = 1;
    cinodo.i_s = 0;
    cinodo.i_atime = time(NULL);
    cinodo.i_ctime = time(NULL);
    cinodo.i_mtime = time(NULL);
    for(int i = 0; i < 15; i++){
        cinodo.i_block[i] = -1;
    }
    cinodo.i_block[0] = 0;
    cinodo.i_type = '0';
    cinodo.i_perm = 777;
    fseek(archivo, sblock.s_inode_start, SEEK_SET);
    fwrite(&cinodo, sizeof(inodo), 1, archivo);

    //CREAR E INICIAR EL BLOQUE DE CARPETAS
    for(int i = 0; i < 4; i++){
        strcpy(ccarpeta.b_content[i].b_name, "-");
        ccarpeta.b_content[i].b_inodo = -1;
    }

    //REGISTRAR EL INODO ACTUAL Y EL DEL PADRE
    strcpy(ccarpeta.b_content[0].b_name, ".");
    ccarpeta.b_content[0].b_inodo = 0;

    strcpy(ccarpeta.b_content[1].b_name, "..");
    ccarpeta.b_content[1].b_inodo = 0;

    //REGISTRAR EL ARCHIVO DE USUARIOS
    strcpy(ccarpeta.b_content[2].b_name, "users.txt");
    ccarpeta.b_content[2].b_inodo = 1;
    fseek(archivo, sblock.s_block_start, SEEK_SET);
    fwrite(&ccarpeta, sizeof(bcarpetas), 1, archivo);
    
    //MARCAR UN NUEVO INODO PARA EL ARCHIVO 
    posLectura = sblock.s_bm_inode_start + sizeof(char);
    fseek(archivo, posLectura, SEEK_SET);
    fwrite(&c, sizeof(char), 1, archivo);

    //MARCAR UN NUEVO BLOQUE PAERA EL ARCHIVO
    char a = 'a';
    posLectura = sblock.s_bm_block_start + sizeof(a);
    fseek(archivo, posLectura, SEEK_SET);
    fwrite(&a, sizeof(char), 1, archivo);

    //LLENAR EL INODO DEL ARCHIVO
    std::string contenido = "1,G,root\n1,U,root,root,123\n";
    cinodo.i_uid = 1;
    cinodo.i_gid = 1;
    cinodo.i_s = contenido.size();
    cinodo.i_atime = time(NULL);
    cinodo.i_ctime = time(NULL);
    cinodo.i_mtime = time(NULL);
    for(int i = 0; i < 15; i++){
        cinodo.i_block[i] = -1;
    }
    cinodo.i_block[0] = 1;
    cinodo.i_type = '1';
    cinodo.i_perm = 777;
    posLectura = sblock.s_inode_start + sizeof(inodo);
    fseek(archivo, posLectura, SEEK_SET);
    fwrite(&cinodo, sizeof(inodo), 1, archivo);
    
    //LLENAR Y ESCRIBIR EL BLOQUE DE ARCHIVOS
    strcpy(carchivo.b_content, contenido.c_str());
    posLectura = sblock.s_block_start + sizeof (barchivos);
    fseek(archivo, posLectura, SEEK_SET);
    fwrite(&carchivo, sizeof(barchivos), 1, archivo);
 
    //REINICIAR EL SISTEMA
    for(int i = 0; i < journal.size(); i++){
        std::vector<std::string> param;
        usuario sesion;
        
        //Leer el contenido del journal
        std::string cont(journal[i].contenido);
        std::string comando(journal[i].comando);
        std::regex coma(",");
        std::vector<std::string> salida(std::sregex_token_iterator(cont.begin(), cont.end(), coma, -1),
                    std::sregex_token_iterator());

        //Crear la sesion
        sesion.id_user = salida[0];
        sesion.user = salida[1];
        sesion.id_grp = salida[2];
        sesion.grupo = salida[3];
        sesion.disco = salida[4];

 
        if(comando == "mkgrp"){
            //Parametros
            param.push_back("mkgrp");

            std::string comandos = "name->";
            comandos.append(salida[5]);
            param.push_back(comandos);
        
            //Comando
            mkgrp(param, discos, sesion);

        }else if(comando == "rmgrp"){
            //Parametros
            param.push_back("rmgrp");

            std::string comandos = "name->";
            comandos.append(salida[5]);
            param.push_back(comandos);
        
            //Comando
            rmgrp(param, discos, sesion);

        }else if(comando == "mkusr"){
            //Parametros
            param.push_back("mkusr");

            std::string comandos = "grp->";
            comandos.append(salida[5]);
            param.push_back(comandos);

            comandos = "usr->";
            comandos.append(salida[6]);
            param.push_back(comandos);

            comandos = "pass->";
            comandos.append(salida[7]);
            param.push_back(comandos);
        
            //Comando
            mkusr(param, discos, sesion);

        }else if(comando == "rmusr"){
            //Parametros
            param.push_back("rmusr");

            std::string comandos = "usr->";
            comandos.append(salida[5]);
            param.push_back(comandos);
        
            //Comando
            rmusr(param, discos, sesion);
        }else if(comando == "chmod"){
            //Parametros
            param.push_back("chmod");

            std::string comandos = "path->";
            comandos.append(journal[i].path);
            param.push_back(comandos);

            comandos = "ugo->";
            comandos.append(salida[6]);
            param.push_back(comandos);

            if(salida[5] == "T"){
                param.push_back("r");
            }
        
            //Comando
            chmod(param, discos, sesion);

        }else if(comando == "mkfile"){
            //Parametros
            param.push_back("mkfile");

            std::string comandos = "path->";
            comandos.append(journal[i].path);
            param.push_back(comandos);

            comandos = "s->";
            comandos.append(salida[6]);
            param.push_back(comandos);

            if(salida[7] != "Error"){
                comandos = "cont->";
                comandos.append(salida[7]);
                param.push_back(comandos);
            }
            
            if(salida[5] == "T"){
                param.push_back("r");
            }
        
            //Comando
            mkfile(param, discos, sesion);

        }else if(comando == "remove"){
            //Parametros
            param.push_back("remove");

            std::string comandos = "path->";
            comandos.append(journal[i].path);
            param.push_back(comandos);
        
            //Comando
            remove(param, discos, sesion);

        }else if(comando == "edit"){
            //Parametros
            param.push_back("edit");

            std::string comandos = "path->";
            comandos.append(journal[i].path);
            param.push_back(comandos);

            comandos = "cont->";
            comandos.append(salida[5]);
            param.push_back(comandos);
        
            //Comando
            edit(param, discos, sesion);

        }else if(comando == "rename"){
            //Parametros
            param.push_back("rename");

            std::string comandos = "path->";
            comandos.append(journal[i].path);
            param.push_back(comandos);

            comandos = "name->";
            comandos.append(salida[5]);
            param.push_back(comandos);
        
            //Comando
            rename(param, discos, sesion);

        }else if(comando == "mkdir"){
            //Parametros
            param.push_back("mkdir");

            std::string comandos = "path->";
            comandos.append(journal[i].path);
            param.push_back(comandos);

            if(salida[5] == "T"){
                param.push_back("p");
            }
        
            //Comando
            mkdir(param, discos, sesion);

        }else if(comando == "copy"){
            //Parametros
            param.push_back("copy");

            std::string comandos = "path->";
            comandos.append(journal[i].path);
            param.push_back(comandos);

            comandos = "destino->";
            comandos.append(salida[5]);
            param.push_back(comandos);
        
            //Comando
            copy(param, discos, sesion);

        }else if(comando == "move"){
            //Parametros
            param.push_back("move");

            std::string comandos = "path->";
            comandos.append(journal[i].path);
            param.push_back(comandos);

            comandos = "destino->";
            comandos.append(salida[5]);
            param.push_back(comandos);
        
            //Comando
            move(param, discos, sesion);

        }else if(comando == "chown"){
            //Parametros
            param.push_back("chown");

            std::string comandos = "path->";
            comandos.append(journal[i].path);
            param.push_back(comandos);

            comandos = "usr->";
            comandos.append(salida[6]);
            param.push_back(comandos);

            if(salida[5] == "T"){
                param.push_back("r");
            }
        
            //Comando
            chown(param, discos, sesion);

        }else if(comando == "chgrp"){
            //Parametros
            param.push_back("chgrp");

            std::string comandos = "grp->";
            comandos.append(salida[6]);
            param.push_back(comandos);

            comandos = "usr->";
            comandos.append(salida[5]);
            param.push_back(comandos);
        
            //Comando
            chgrp(param, discos, sesion);

        }
    }
        
    std::cout << "MENSAJE: Sistema recuperado correctamente." << std::endl;
    fclose(archivo);
}
