#include "chgrp.h"

void chgrp(std::vector<std::string> &parametros, std::vector<disco> &discos, usuario &sesion){
    //VERIFICAR QUE EL USUARIO ROOT ESTÉ LOGUEADO
    if(sesion.user != "root"){
        std::cout << "ERROR: Este comando solo funciona con el usuario root." << std::endl;
        return;
    }

    //VARIABLES
    bool paramFlag = true;                     //Indica si se cumplen con los parametros del comando
    bool required = true;                      //Indica si vienen los parametros obligatorios
    bool valid = true;                         //Verifica que los valores de los parametros sean correctos
    FILE *archivo;                             //Sirve para verificar que el archivo exista
    std::string nombre = "";                   //Atributo usr
    std::string grupo = "";                    //Atributo grupo
    std::string diskName = "";                 //Nombre del disco
    int posDisco = -1;                         //Posicion del disco dentro del vector
    int posParticion = -1;                     //Posicion de la particion dentro del vector del disco
    int posInicio;                             //Posicion donde inicia la particion
    int posLectura;                            //Para determinar la posicion de lectura en disco
    int tamaño;                                //Tamaño de la particion que se va a formatear
    int inodo_buscado = -1;                    //Numero de Inodo del archivo users.txt
    sbloque sblock;                            //Para leer el superbloque
    inodo linodo;                              //Para el manejo de los inodos
    bcarpetas lcarpeta;                        //Para el manejo de bloques de carpetas
    barchivos larchivo;                        //Para el manejo de bloques de archivo
    bapuntadores lapuntador;                   //Para el manejo de bloques de apuntadores simples
    bapuntadores lapuntador_doble;             //Para el manejo de bloques de apuntadores dobles
    bapuntadores lapuntador_triple;            //Para el manejo de bloques de apuntadores triples
    std::string texto = "";                    //Para almacenar el contenido del archivo de usuarios
    bool existe_usuario = false;               //Indica si se encontró el usuario
    bool existe_grupo = false;                 //Indica si se encontró el grupo
    int bloque_inicial;                        //Numero de bloque que contiene el inicio del archivo  
    std::string escribir;                      //Variable para almacenar los cortes del archivo   
    std::string editar;                        //Para editar la linea con la instruccion 

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

        if(tag == "usr"){
            nombre = value;
        }else if(tag == "grp"){
            grupo = value;
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
    if(nombre == ""){
        required = false;
    }

    if(!required){
        std::cout << "ERROR: La instrucción mkgrp carece de todos los parametros obligatorios." << std::endl;
    }

    //REMOVER NUMEROS DEL ID PARA OBTENER EL NOMBRE DEL DISCO
    int posicion = 0;
    for(int i = 0; i< sesion.disco.length(); i++){
        if(isdigit(sesion.disco[i])){
            posicion++;
        }else{
            break;
        }
    }
    diskName = sesion.disco.substr(posicion, sesion.disco.length() - 1);

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
        if(temp.id == sesion.disco){
            posParticion = i;
            break;
        }
    }

    if(posParticion == -1){
        std::cout << "ERROR: La particion que desea formatear no existe." << std::endl;
        return;
    }

    //REVISAR SI EXISTE EL ARCHIVO 
    montada &formatear = tempD.particiones[posParticion];
    archivo = fopen(tempD.ruta.c_str(), "r+b");

    if(archivo == NULL){
        std::cout << "ERROR: El disco fisico no existe." << std::endl;
        return;
    }

    //DETERMINAR LA POSICION DE INICIO Y EL TAMAÑO DE LA PARTICION
    if(formatear.posMBR != -1){
        MBR mbr;
        fseek(archivo, 0,SEEK_SET);
        fread(&mbr, sizeof(MBR), 1, archivo);
        posInicio = mbr.mbr_partition[formatear.posMBR].part_start;
        tamaño = mbr.mbr_partition[formatear.posMBR].part_s;
    }else{
        EBR ebr;
        fseek(archivo, formatear.posEBR, SEEK_SET);
        fread(&ebr, sizeof(EBR), 1, archivo);
        posInicio = ebr.part_start;
        tamaño = ebr.part_s;
    }

    //LEER EL SUPERBLOQUE
    fseek(archivo, posInicio, SEEK_SET);
    fread(&sblock, sizeof(sbloque), 1, archivo);

    //LEER EL INODO RAIZ
    posLectura = sblock.s_inode_start;
    fseek(archivo, posLectura, SEEK_SET);
    fread(&linodo, sizeof(inodo), 1, archivo);

    //BUSCAR EL ARCHIVO DE USUARIOS 
    for(int i = 0; i < 15; i++){
        if(inodo_buscado != -1){
            break;
        }

        if(linodo.i_block[i] == -1){
            continue;
        }

        if(i == 12){
            //Recorrer el bloque de apuntadores simple
            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * linodo.i_block[i]);
            fseek(archivo, posLectura, SEEK_SET);
            fread(&lapuntador, sizeof(bapuntadores), 1, archivo);

            for(int j = 0; j < 16; j++){
                if(lapuntador.b_pointers[j] == -1){
                    continue;
                }

                posLectura = sblock.s_block_start + (sizeof(bcarpetas) * lapuntador.b_pointers[j]);
                fseek(archivo, posLectura, SEEK_SET);
                fread(&lcarpeta, sizeof(bcarpetas), 1, archivo);

                for(int k = 0; k < 4; k++){
                    std::string carpeta(lcarpeta.b_content[k].b_name);

                    if(carpeta == "users.txt"){
                        inodo_buscado = lcarpeta.b_content[k].b_inodo;
                        break;
                    }
                }
            }
        }else if(i == 13){
            //Recorrer el bloque de apuntadores doble
            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * linodo.i_block[i]);
            fseek(archivo, posLectura, SEEK_SET);
            fread(&lapuntador_doble, sizeof(bapuntadores), 1, archivo);

            for(int j = 0; j < 16; j++){
                if(lapuntador_doble.b_pointers[j] == -1){
                    continue;
                }

                posLectura = sblock.s_block_start + (sizeof(bapuntadores) * lapuntador_doble.b_pointers[j]);
                fseek(archivo, posLectura, SEEK_SET);
                fread(&lapuntador, sizeof(bapuntadores), 1, archivo);

                for(int k = 0; k < 16; k++){
                    if(lapuntador.b_pointers[k] == -1){
                        continue;
                    }

                    posLectura = sblock.s_block_start + (sizeof(bcarpetas) * lapuntador.b_pointers[k]);
                    fseek(archivo, posLectura, SEEK_SET);
                    fread(&lcarpeta, sizeof(bcarpetas), 1, archivo);

                    for(int l = 0; l < 4; l++){
                        std::string carpeta(lcarpeta.b_content[l].b_name);

                        if(carpeta == "users.txt"){
                            inodo_buscado = lcarpeta.b_content[l].b_inodo;
                            break;
                        }
                    }
                }
            }
        }else if(i == 14){
            //Recorrer el bloque de apuntadores triple
            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * linodo.i_block[i]);
            fseek(archivo, posLectura, SEEK_SET);
            fread(&lapuntador_triple, sizeof(bapuntadores), 1, archivo);

            for(int j = 0; j < 16; j++){
                if(lapuntador_triple.b_pointers[j] == -1){
                    continue;
                }

                posLectura = sblock.s_block_start + (sizeof(bapuntadores) * lapuntador_triple.b_pointers[j]);
                fseek(archivo, posLectura, SEEK_SET);
                fread(&lapuntador_doble, sizeof(bapuntadores), 1, archivo);

                for(int k = 0; k < 16; k++){
                    if(lapuntador_doble.b_pointers[k] == -1){
                        continue;
                    }

                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * lapuntador_doble.b_pointers[k]);
                    fseek(archivo, posLectura, SEEK_SET);
                    fread(&lapuntador, sizeof(bapuntadores), 1, archivo);

                    for(int l = 0; l < 16; l++){
                        if(lapuntador.b_pointers[l] == -1){
                            continue;
                        }

                        posLectura = sblock.s_block_start + (sizeof(bcarpetas) * lapuntador.b_pointers[l]);
                        fseek(archivo, posLectura, SEEK_SET);
                        fread(&lcarpeta, sizeof(bcarpetas), 1, archivo);

                        for(int m = 0; m < 4; m++){
                            std::string carpeta(lcarpeta.b_content[m].b_name);

                            if(carpeta == "users.txt"){
                                inodo_buscado = lcarpeta.b_content[m].b_inodo;
                                break;
                            }
                        }
                    }
                }
            }
        }else{
            posLectura = sblock.s_block_start + (sizeof(bcarpetas) * linodo.i_block[i]);
            fseek(archivo, posLectura, SEEK_SET);
            fread(&lcarpeta, sizeof(bcarpetas), 1, archivo);

            for(int j = 0; j < 4; j++){
                std::string carpeta(lcarpeta.b_content[j].b_name);
    
                if(carpeta == "users.txt"){
                    inodo_buscado = lcarpeta.b_content[j].b_inodo;
                    break;
                }
            }
        }
    }

    if(inodo_buscado == -1){
        std::cout << "ERROR: No se encontró el archivo de usuarios." << std::endl;
        fclose(archivo);
        return;
    }

    //LEER EL INODO DEL ARCHIVO
    posLectura = sblock.s_inode_start + (sizeof(inodo) * inodo_buscado);
    fseek(archivo, posLectura, SEEK_SET);
    fread(&linodo, sizeof(inodo), 1, archivo);

    //LEER EL ARCHIVO DE USUARIOS 
    std::string temp; 
    for(int i = 0; i < 15; i++){
        if(linodo.i_block[i] == -1){
            continue;
        }

        if(i == 12){
            //Recorrer el bloque de apuntadores simple
            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * linodo.i_block[i]);
            fseek(archivo, posLectura, SEEK_SET);
            fread(&lapuntador, sizeof(bapuntadores), 1, archivo);

            for(int j = 0; j < 16; j++){
                if(lapuntador.b_pointers[j] == -1){
                    continue;
                }

                posLectura = sblock.s_block_start + (sizeof(barchivos) * lapuntador.b_pointers[j]);
                fseek(archivo, posLectura, SEEK_SET);
                fread(&larchivo, sizeof(barchivos), 1, archivo);


                temp = larchivo.b_content; 
                texto += temp;
            }
        }else if(i == 13){
            //Recorrer el bloque de apuntadores simple
            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * linodo.i_block[i]);
            fseek(archivo, posLectura, SEEK_SET);
            fread(&lapuntador_doble, sizeof(bapuntadores), 1, archivo);

            for(int j = 0; j < 16; j++){
                if(lapuntador_doble.b_pointers[j] == -1){
                    continue;
                }

                posLectura = sblock.s_block_start + (sizeof(bapuntadores) * lapuntador_doble.b_pointers[j]);
                fseek(archivo, posLectura, SEEK_SET);
                fread(&lapuntador, sizeof(bapuntadores), 1, archivo);

                for(int k = 0; k < 16; k++){
                    if(lapuntador.b_pointers[k] == -1){
                        continue;
                    }

                    posLectura = sblock.s_block_start + (sizeof(barchivos) * lapuntador.b_pointers[k]);
                    fseek(archivo, posLectura, SEEK_SET);
                    fread(&larchivo, sizeof(barchivos), 1, archivo);

                    temp = larchivo.b_content; 
                    texto += temp;
                }
            }
        }else if(i == 14){
            //Recorrer el bloque de apuntadores simple
            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * linodo.i_block[i]);
            fseek(archivo, posLectura, SEEK_SET);
            fread(&lapuntador_triple, sizeof(bapuntadores), 1, archivo);

            for(int j = 0; j < 16; j++){
                if(lapuntador_triple.b_pointers[j] == -1){
                    continue;
                }

                posLectura = sblock.s_block_start + (sizeof(bapuntadores) * lapuntador_triple.b_pointers[j]);
                fseek(archivo, posLectura, SEEK_SET);
                fread(&lapuntador_doble, sizeof(bapuntadores), 1, archivo);

                for(int k = 0; k < 16; k++){
                    if(lapuntador_doble.b_pointers[k] == -1){
                        continue;
                    }

                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * lapuntador_doble.b_pointers[k]);
                    fseek(archivo, posLectura, SEEK_SET);
                    fread(&lapuntador, sizeof(bapuntadores), 1, archivo);

                    for(int l = 0; l < 16; l++){
                        if(lapuntador.b_pointers[l] == -1){
                            continue;
                        }

                        posLectura = sblock.s_block_start + (sizeof(barchivos) * lapuntador.b_pointers[l]);
                        fseek(archivo, posLectura, SEEK_SET);
                        fread(&larchivo, sizeof(barchivos), 1, archivo);

                        temp = larchivo.b_content; 
                        texto += temp;
                    }
                }
            }
        }else{
            posLectura = sblock.s_block_start + (sizeof(barchivos) * linodo.i_block[i]);
            fseek(archivo, posLectura, SEEK_SET);
            fread(&larchivo, sizeof(barchivos), 1, archivo);

            temp = larchivo.b_content; 
            texto += temp;
        }
    }

    //SEPARAR EL ARCHIVO POR LINEAS
    std::regex espacio("\n");
    std::regex coma(",");
    std::vector<std::string> lineas(std::sregex_token_iterator(texto.begin(), texto.end(), espacio, -1),
        std::sregex_token_iterator());

    //BUSCAR EL GRUPO
    for(int i = 0; i < lineas.size(); i++){
        //Separar por comas los atributos
        std::vector<std::string> atributos(std::sregex_token_iterator(lineas[i].begin(), lineas[i].end(), coma, -1),
            std::sregex_token_iterator());
        if(atributos.size() == 3){  //Los grupos tienen tres parametros
            if(atributos[0] != "0"){
                if(atributos[2] == grupo){
                    existe_grupo = true;
                    break;
                }
            }else{
                if(atributos[2] == grupo){
                    std::cout << "ERROR: El grupo que busca fue eliminado." << std::endl;
                    break;
                }
            }
        }
    }

    //BUSCAR EL USUARIO
    for(int i = 0; i < lineas.size(); i++){
        std::string edit = "";
        //Separar por comas los atributos
        std::vector<std::string> atributos(std::sregex_token_iterator(lineas[i].begin(), lineas[i].end(), coma, -1),
            std::sregex_token_iterator());
        if(atributos.size() == 5){  //Los usuarios tienen cinco parametros
            if(atributos[0] != "0"){
                if(atributos[3] == nombre){
                    edit = atributos[0];
                    edit += ",";
                    edit += atributos[1];
                    edit += ",";
                    edit += grupo;
                    edit += ",";
                    edit += atributos[3];
                    edit += ",";
                    edit += atributos[4];
                    existe_usuario = true;
                }
            }
        }

        if(existe_usuario){
            lineas[i] = edit;
            break;
        }
    }

    if(!existe_usuario){
        std::cout << "ERROR: El usuario no existe." << std::endl;
        fclose(archivo);
        return;
    }

    if(!existe_grupo){
        std::cout << "ERROR: El grupo no existe." << std::endl;
        fclose(archivo);
        return;
    }
    //RECONSTRUIR EL ARCHIVO EDITADO
    texto = "";
    for(int i = 0; i < lineas.size(); i++){
        texto += lineas[i];
        texto += "\n";
    }
    
    //ESCRIBIR EN EL JOURNAL EL COMANDO
    if(sblock.s_filesystem_type == 3){
        registro creacion;
        int posRegistro = -1;

        //Buscar un Journal Vacio
        posLectura = posInicio + sizeof(sbloque);
        for(int i = 0; i < sblock.s_inodes_count; i++){
            fseek(archivo, posLectura, SEEK_SET);
            fread(&creacion, sizeof(registro), 1, archivo);

            if(creacion.comando[0] == '\0'){
                posRegistro = posLectura;
                break;
            }else{
                posLectura += sizeof(registro);
            }
        }

        if(posRegistro != -1){
            std::string contenido = nombre;
            contenido += ",";
            contenido += grupo;
            strcpy(creacion.comando ,"chgrp");
            strcpy(creacion.path ,"/");
            strcpy(creacion.nombre ,"");
            strcpy(creacion.contenido, contenido.c_str());
            creacion.fecha = time(NULL);
            fseek(archivo, posRegistro, SEEK_SET);
            fwrite(&creacion, sizeof(registro), 1, archivo);
        }
    }

    //REINICIAR TODOS LOS ESPACIOS DEL INODO
    bloque_inicial = linodo.i_block[0];
    for(int i = 0; i < 15; i++){
        linodo.i_block[i] = -1;
    }

    //REESCRIBIR EL ARCHIVO DE USUARIOS 
    bool continuar = true;
    posicion = 0;
    char c;

    while(continuar){
        bool revisar = true;
        barchivos earchivo;
        bapuntadores eapuntador;
        bapuntadores eapuntador_doble;
        bapuntadores eapuntador_triple;

        for(int z = 0; z < 16; z++){
            eapuntador.b_pointers[z] = -1;
            eapuntador_doble.b_pointers[z] = -1;
            eapuntador_triple.b_pointers[z] = -1;
        }

        if(texto.size() > 63){
            escribir = texto.substr(0, 63); 
            texto = texto.substr(63, texto.length()-1);
        }else{
            escribir = texto;
            continuar = false;
        }
        
        while(revisar){

            if(posicion == 12){
                if(linodo.i_block[posicion] == -1){
                    //Escribir en el bitmap el bloque de apuntadores
                    c = 'p';
                    posLectura = sblock.s_bm_block_start + (sizeof(char) * bloque_inicial);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&c, sizeof(char), 1, archivo);

                    //Escribir en el bitmap el bloque de archivos
                    c = 'a';
                    bloque_inicial += 1;
                    posLectura = sblock.s_bm_block_start + (sizeof(char) * bloque_inicial);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&c, sizeof(char), 1, archivo);

                    //Crear y escribir el bloque de archivos
                    strcpy(earchivo.b_content, escribir.c_str());
                    posLectura = sblock.s_block_start + (sizeof(barchivos) * bloque_inicial);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&earchivo, sizeof(barchivos), 1, archivo);

                    //Crear y escribir el bloque de apuntadores
                    eapuntador.b_pointers[0] = bloque_inicial;
                    bloque_inicial -= 1;
                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * bloque_inicial);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&eapuntador, sizeof(bapuntadores), 1, archivo);

                    //Actualizar el inodo
                    linodo.i_block[posicion] = bloque_inicial;
                    bloque_inicial += 2; 
                    revisar = false;
                }else{
                    int espacio = -1;

                    //Leer el inodo de apuntadores
                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * linodo.i_block[posicion]);
                    fseek(archivo, posLectura, SEEK_SET);
                    fread(&lapuntador, sizeof(bapuntadores), 1, archivo);

                    //Revisar si hay espacio
                    for(int i = 0; i < 16; i++){
                        if(lapuntador.b_pointers[i] == -1){
                            espacio = i;
                            break;
                        }
                    }

                    if(espacio == -1){
                        posicion += 1;
                    }else{
                        //Escribir en el bitmap el bloque
                        c = 'a';
                        posLectura = sblock.s_bm_block_start + (sizeof(char) * bloque_inicial);
                        fseek(archivo, posLectura, SEEK_SET);
                        fwrite(&c, sizeof(char), 1, archivo);

                        //Crear y escribir el bloque
                        strcpy(earchivo.b_content, escribir.c_str());
                        posLectura = sblock.s_block_start + (sizeof(barchivos) * bloque_inicial);
                        fseek(archivo, posLectura, SEEK_SET);
                        fwrite(&earchivo, sizeof(barchivos), 1, archivo);

                        //Actualizar y escribir el bloque de apuntadores
                        lapuntador.b_pointers[espacio] = bloque_inicial;
                        posLectura = sblock.s_block_start + (sizeof(bapuntadores) * linodo.i_block[posicion]);;
                        fseek(archivo, posLectura, SEEK_SET);
                        fwrite(&lapuntador, sizeof(bapuntadores), 1, archivo);

                        //Actualizar variables
                        bloque_inicial += 1; 
                        revisar = false;
                    }
                }

            }else if(posicion == 13){
                if(linodo.i_block[posicion] == -1){
                    //Escribir en el bitmap el bloque de apuntadores doble
                    c = 'p';
                    posLectura = sblock.s_bm_block_start + (sizeof(char) * bloque_inicial);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&c, sizeof(char), 1, archivo);

                    //Escribir en el bitmap el bloque de apuntadores
                    bloque_inicial += 1;
                    posLectura = sblock.s_bm_block_start + (sizeof(char) * bloque_inicial);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&c, sizeof(char), 1, archivo);

                    //Escribir en el bitmap el bloque de archivos
                    c = 'a';
                    bloque_inicial += 1;
                    posLectura = sblock.s_bm_block_start + (sizeof(char) * bloque_inicial);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&c, sizeof(char), 1, archivo);

                    //Crear y escribir el bloque de archivos
                    strcpy(earchivo.b_content, escribir.c_str());
                    posLectura = sblock.s_block_start + (sizeof(barchivos) * bloque_inicial);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&earchivo, sizeof(barchivos), 1, archivo);

                    //Crear y escribir el bloque de apuntadores 
                    eapuntador.b_pointers[0] = bloque_inicial;
                    bloque_inicial -= 1;
                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * bloque_inicial);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&eapuntador, sizeof(bapuntadores), 1, archivo);

                    //Crear y escribir el bloque de apuntadores dobles
                    eapuntador_doble.b_pointers[0] = bloque_inicial;
                    bloque_inicial -= 1;
                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * bloque_inicial);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&eapuntador_doble, sizeof(bapuntadores), 1, archivo);

                    //Actualizar el inodo
                    linodo.i_block[posicion] = bloque_inicial;
                    bloque_inicial += 3; 
                    revisar = false;
                }else{
                    int espacio = -1;

                    //Leer el inodo de apuntadores dobles
                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * linodo.i_block[posicion]);
                    fseek(archivo, posLectura, SEEK_SET);
                    fread(&lapuntador_doble, sizeof(bapuntadores), 1, archivo);

                    //Revisar el inodo doble
                    for(int i = 0; i < 16; i++){
                        if(lapuntador_doble.b_pointers[i] == -1){
                            //Escribir en el bitmap el bloque de apuntadores
                            c = 'p';
                            posLectura = sblock.s_bm_block_start + (sizeof(char) * bloque_inicial);
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&c, sizeof(char), 1, archivo);

                            //Escribir en el bitmap el bloque de archivos
                            c = 'a';
                            bloque_inicial += 1;
                            posLectura = sblock.s_bm_block_start + (sizeof(char) * bloque_inicial);
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&c, sizeof(char), 1, archivo);

                            //Crear y escribir el bloque de archivos
                            strcpy(earchivo.b_content, escribir.c_str());
                            posLectura = sblock.s_block_start + (sizeof(barchivos) * bloque_inicial);
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&earchivo, sizeof(barchivos), 1, archivo);

                            //Crear y escribir el bloque de apuntadores
                            eapuntador.b_pointers[0] = bloque_inicial;
                            bloque_inicial -= 1;
                            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * bloque_inicial);
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&eapuntador, sizeof(bapuntadores), 1, archivo);

                            //Actualizar el apuntador
                            lapuntador_doble.b_pointers[i] = bloque_inicial;
                            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * linodo.i_block[posicion]);;
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&lapuntador_doble, sizeof(bapuntadores), 1, archivo);

                            //Actualizar Variables
                            bloque_inicial += 2; 
                            revisar = false;
                            break;

                        }else{
                            //Leer el inodo de apuntadores 
                            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * lapuntador_doble.b_pointers[i]);
                            fseek(archivo, posLectura, SEEK_SET);
                            fread(&lapuntador, sizeof(bapuntadores), 1, archivo);

                            //Buscar si hay espacio
                            for(int j = 0; j < 16; j++){
                                if(lapuntador.b_pointers[j] == -1){
                                    espacio = j;
                                    break;
                                }
                            }

                            if(espacio == -1){
                                if(i == 15){
                                    posicion += 1;
                                }
                            }else{
                                //Escribir en el bitmap el bloque
                                c = 'a';
                                posLectura = sblock.s_bm_block_start + (sizeof(char) * bloque_inicial);
                                fseek(archivo, posLectura, SEEK_SET);
                                fwrite(&c, sizeof(char), 1, archivo);

                                //Crear y escribir el bloque
                                strcpy(earchivo.b_content, escribir.c_str());
                                posLectura = sblock.s_block_start + (sizeof(barchivos) * bloque_inicial);
                                fseek(archivo, posLectura, SEEK_SET);
                                fwrite(&earchivo, sizeof(barchivos), 1, archivo);

                                //Actualizar y escribir el bloque de apuntadores
                                lapuntador.b_pointers[espacio] = bloque_inicial;
                                posLectura = sblock.s_block_start + (sizeof(bapuntadores) * lapuntador_doble.b_pointers[i]);;
                                fseek(archivo, posLectura, SEEK_SET);
                                fwrite(&lapuntador, sizeof(bapuntadores), 1, archivo);

                                //Actualizar variables
                                bloque_inicial += 1; 
                                revisar = false;
                                break;
                            }
                        }
                    }
                }
            }else if(posicion == 14){
                if(linodo.i_block[posicion] == -1){
                    //Escribir en el bitmap el bloque de apuntadores triple
                    c = 'p';
                    posLectura = sblock.s_bm_block_start + (sizeof(char) * bloque_inicial);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&c, sizeof(char), 1, archivo);

                    //Escribir en el bitmap el bloque de apuntadores doble
                    bloque_inicial += 1;
                    posLectura = sblock.s_bm_block_start + (sizeof(char) * bloque_inicial);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&c, sizeof(char), 1, archivo);

                    //Escribir en el bitmap el bloque de apuntadores
                    bloque_inicial += 1;
                    posLectura = sblock.s_bm_block_start + (sizeof(char) * bloque_inicial);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&c, sizeof(char), 1, archivo);

                    //Escribir en el bitmap el bloque de archivos
                    c = 'a';
                    bloque_inicial += 1;
                    posLectura = sblock.s_bm_block_start + (sizeof(char) * bloque_inicial);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&c, sizeof(char), 1, archivo);

                    //Crear y escribir el bloque de archivos
                    strcpy(earchivo.b_content, escribir.c_str());
                    posLectura = sblock.s_block_start + (sizeof(barchivos) * bloque_inicial);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&earchivo, sizeof(barchivos), 1, archivo);

                    //Crear y escribir el bloque de apuntadores 
                    eapuntador.b_pointers[0] = bloque_inicial;
                    bloque_inicial -= 1;
                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * bloque_inicial);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&eapuntador, sizeof(bapuntadores), 1, archivo);

                    //Crear y escribir el bloque de apuntadores dobles
                    eapuntador_doble.b_pointers[0] = bloque_inicial;
                    bloque_inicial -= 1;
                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * bloque_inicial);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&eapuntador_doble, sizeof(bapuntadores), 1, archivo);

                    //Crear y escribir el bloque de apuntadores triples
                    eapuntador_triple.b_pointers[0] = bloque_inicial;
                    bloque_inicial -= 1;
                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * bloque_inicial);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&eapuntador_triple, sizeof(bapuntadores), 1, archivo);

                    //Actualizar el inodo
                    linodo.i_block[posicion] = bloque_inicial;
                    bloque_inicial += 4; 
                    revisar = false;
                }else{
                    int espacio = -1;
                    bool salir = false;

                    //Leer el inodo de apuntadores triple
                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * linodo.i_block[posicion]);
                    fseek(archivo, posLectura, SEEK_SET);
                    fread(&lapuntador_triple, sizeof(bapuntadores), 1, archivo);

                    //Revisar el inodo triple
                    for(int i = 0; i < 16; i++){
                        if(lapuntador_triple.b_pointers[i] == -1){
                            //Escribir en el bitmap el bloque de apuntadores doble
                            c = 'p';
                            posLectura = sblock.s_bm_block_start + (sizeof(char) * bloque_inicial);
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&c, sizeof(char), 1, archivo);

                            //Escribir en el bitmap el bloque de apuntadores
                            bloque_inicial += 1;
                            posLectura = sblock.s_bm_block_start + (sizeof(char) * bloque_inicial);
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&c, sizeof(char), 1, archivo);

                            //Escribir en el bitmap el bloque de archivos
                            c = 'a';
                            bloque_inicial += 1;
                            posLectura = sblock.s_bm_block_start + (sizeof(char) * bloque_inicial);
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&c, sizeof(char), 1, archivo);

                            //Crear y escribir el bloque de archivos
                            strcpy(earchivo.b_content, escribir.c_str());
                            posLectura = sblock.s_block_start + (sizeof(barchivos) * bloque_inicial);
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&earchivo, sizeof(barchivos), 1, archivo);

                            //Crear y escribir el bloque de apuntadores
                            eapuntador.b_pointers[0] = bloque_inicial;
                            bloque_inicial -= 1;
                            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * bloque_inicial);
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&eapuntador, sizeof(bapuntadores), 1, archivo);

                            //Crear y escribir el bloque de apuntadores dobles
                            eapuntador_doble.b_pointers[0] = bloque_inicial;
                            bloque_inicial -= 1;
                            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * bloque_inicial);
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&eapuntador_doble, sizeof(bapuntadores), 1, archivo);

                            //Actualizar el apuntador
                            lapuntador_triple.b_pointers[i] = bloque_inicial;
                            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * linodo.i_block[posicion]);;
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&lapuntador_triple, sizeof(bapuntadores), 1, archivo);

                            //Actualizar Variables
                            bloque_inicial += 3; 
                            revisar = false;
                            break;

                        }else{
                            //Leer el bloque de apuntadores 
                            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * lapuntador_triple.b_pointers[i]);
                            fseek(archivo, posLectura, SEEK_SET);
                            fread(&lapuntador_doble, sizeof(bapuntadores), 1, archivo);

                            //Recorrer el bloque de apuntadores dobles 
                            for(int j = 0; j < 16; j++){
                                if(lapuntador_doble.b_pointers[j] == -1){
                                    //Escribir en el bitmap el bloque de apuntadores
                                    c = 'p';
                                    posLectura = sblock.s_bm_block_start + (sizeof(char) * bloque_inicial);
                                    fseek(archivo, posLectura, SEEK_SET);
                                    fwrite(&c, sizeof(char), 1, archivo);

                                    //Escribir en el bitmap el bloque de archivos
                                    c = 'a';
                                    bloque_inicial += 1;
                                    posLectura = sblock.s_bm_block_start + (sizeof(char) * bloque_inicial);
                                    fseek(archivo, posLectura, SEEK_SET);
                                    fwrite(&c, sizeof(char), 1, archivo);

                                    //Crear y escribir el bloque de archivos
                                    strcpy(earchivo.b_content, escribir.c_str());
                                    posLectura = sblock.s_block_start + (sizeof(barchivos) * bloque_inicial);
                                    fseek(archivo, posLectura, SEEK_SET);
                                    fwrite(&earchivo, sizeof(barchivos), 1, archivo);

                                    //Crear y escribir el bloque de apuntadores
                                    eapuntador.b_pointers[0] = bloque_inicial;
                                    bloque_inicial -= 1;
                                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * bloque_inicial);
                                    fseek(archivo, posLectura, SEEK_SET);
                                    fwrite(&eapuntador, sizeof(bapuntadores), 1, archivo);

                                    //Actualizar el apuntador
                                    lapuntador_doble.b_pointers[j] = bloque_inicial;
                                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * lapuntador_triple.b_pointers[i]);;
                                    fseek(archivo, posLectura, SEEK_SET);
                                    fwrite(&lapuntador_doble, sizeof(bapuntadores), 1, archivo);

                                    //Actualizar Variables
                                    bloque_inicial += 2; 
                                    revisar = false;
                                    salir = true;
                                    break;
                                    
                                }else{
                                    //Leer el inodo de apuntadores 
                                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * lapuntador_doble.b_pointers[j]);
                                    fseek(archivo, posLectura, SEEK_SET);
                                    fread(&lapuntador, sizeof(bapuntadores), 1, archivo);

                                    //Buscar si hay espacio
                                    for(int k = 0; k < 16; k++){
                                        if(lapuntador.b_pointers[k] == -1){
                                            espacio = k;
                                            break;
                                        }
                                    }

                                    if(espacio != -1){
                                        //Escribir en el bitmap el bloque
                                        c = 'a';
                                        posLectura = sblock.s_bm_block_start + (sizeof(char) * bloque_inicial);
                                        fseek(archivo, posLectura, SEEK_SET);
                                        fwrite(&c, sizeof(char), 1, archivo);

                                        //Crear y escribir el bloque
                                        strcpy(earchivo.b_content, escribir.c_str());
                                        posLectura = sblock.s_block_start + (sizeof(barchivos) * bloque_inicial);
                                        fseek(archivo, posLectura, SEEK_SET);
                                        fwrite(&earchivo, sizeof(barchivos), 1, archivo);

                                        //Actualizar y escribir el bloque de apuntadores
                                        lapuntador.b_pointers[espacio] = bloque_inicial;
                                        posLectura = sblock.s_block_start + (sizeof(bapuntadores) * lapuntador_doble.b_pointers[j]);;
                                        fseek(archivo, posLectura, SEEK_SET);
                                        fwrite(&lapuntador, sizeof(bapuntadores), 1, archivo);

                                        //Actualizar variables
                                        bloque_inicial += 1; 
                                        revisar = false;
                                        salir = true; 
                                        break;
                                    }

                                }
                            }

                            
                        }

                        if(salir){
                            break;
                        }
                    }
                }

            }else{
                //Escribir en el bitmap el bloque
                c = 'a';
                posLectura = sblock.s_bm_block_start + (sizeof(char) * bloque_inicial);
                fseek(archivo, posLectura, SEEK_SET);
                fwrite(&c, sizeof(char), 1, archivo);

                //Crear y escribir el bloque
                strcpy(earchivo.b_content, escribir.c_str());
                posLectura = sblock.s_block_start + (sizeof(barchivos) * bloque_inicial);
                fseek(archivo, posLectura, SEEK_SET);
                fwrite(&earchivo, sizeof(barchivos), 1, archivo);

                //Actualizar el inodo
                linodo.i_block[posicion] = bloque_inicial;
                posicion += 1;
                bloque_inicial += 1;
                revisar = false;
            }
        }
    }

    //REESCRIBIR EL INODO CON TODOS LOS CAMBIOS
    linodo.i_mtime = time(NULL);
    posLectura = sblock.s_inode_start + (sizeof(inodo) * inodo_buscado);
    fseek(archivo, posLectura, SEEK_SET);
    fwrite(&linodo, sizeof(inodo), 1, archivo);
 
    std::cout << "MENSAJE: Cambio de grupo efectuado correctamente." << std::endl;
    fclose(archivo);
}