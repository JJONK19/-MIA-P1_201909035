#include "remove.h"

void remove(std::vector<std::string> &parametros, std::vector<disco> &discos, usuario &sesion){
    
    //VERIFICAR QUE EL USUARIO ESTÉ LOGUEADO
    if(sesion.user[0] == '\0'){
        std::cout << "ERROR: Debe haber una sesión iniciada para usar este comando." << std::endl;
        return;
    }

    //VARIABLES
    bool paramFlag = true;                     //Indica si se cumplen con los parametros del comando
    bool required = true;                      //Indica si vienen los parametros obligatorios
    bool valid = true;                         //Verifica que los valores de los parametros sean correctos
    FILE *archivo;                             //Sirve para verificar que el archivo exista
    std::string ruta = "";                     //Atributo path
    std::string diskName = "";                 //Nombre del disco
    int posDisco = -1;                         //Posicion del disco dentro del vector
    int posParticion = -1;                     //Posicion de la particion dentro del vector del disco
    int posInicio;                             //Posicion donde inicia la particion
    int posLectura;                            //Para determinar la posicion de lectura en disco
    sbloque sblock;                            //Para leer el superbloque
    inodo linodo;                              //Para el manejo de los inodos
    bcarpetas lcarpeta;                        //Para el manejo de bloques de carpetas
    barchivos larchivo;                        //Para el manejo de bloques de archivo
    bapuntadores lapuntador;                   //Para el manejo de bloques de apuntadores simples
    bapuntadores lapuntador_doble;             //Para el manejo de bloques de apuntadores dobles
    bapuntadores lapuntador_triple;            //Para el manejo de bloques de apuntadores triples
    bool continuar = true;                     //Usado como bandera al buscar la ruta
    std::string nombre_archivo;                //Nombre del archivo sin la ruta
    int inodo_leido = -1;                      //Numero de inodo que se está leyendo
    std::string contenido = "";                //Texto que se va a escribir en el archivo
    int bloques = -1;                          //Numero de bloques que va a usar el archivo
    std::vector<int> borrar_bloques;           //Vector con la lista de bloques a borrar
    std::vector<int> borrar_inodos;            //Vector con la lista de inodos a borra
    int inodo_padre = 0;                      //Inodo de la carpeta que contiene el archivo/carpeta a borrar                      
    bool borrar_padre;                         //Indica si se va a eliminar de la carpeta de origen

    //COMPROBACIÓN DE PARAMETROS
    for(int i = 1; i < parametros.size(); i++){
        std::string &temp = parametros[i];

        std::vector<std::string> salida(std::sregex_token_iterator(temp.begin(), temp.end(), flecha, -1),
                    std::sregex_token_iterator());

        //Se separa en dos para manejar el atributo -r
        std::string &tag = salida[0];
        std::string &value = salida[1];

        //Pasar a minusculas
        transform(tag.begin(), tag.end(), tag.begin(),[](unsigned char c){
            return tolower(c);
        });

        if(tag == "path"){
            ruta = value;
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
    if(ruta == ""){
        required = false;
    }

    if(!required){
        std::cout << "ERROR: La instrucción remove carece de todos los parametros obligatorios." << std::endl;
    }

    //PREPARACIÓN DE PARAMETROS - Extraer de la ruta el nombre del archivo y eliminarlo
    nombre_archivo = ruta.substr(ruta.find_last_of("\\/") + 1, ruta.size() - 1);
    
    //PREPARACIÓN DE PARAMETROS - Separar los nombres que vengan en la ruta.
    std::regex separador("/");
    std::vector<std::string> path(std::sregex_token_iterator(ruta.begin(), ruta.end(), separador, -1),
                    std::sregex_token_iterator());

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

    //LEER EL INODO RAIZ
    posLectura = sblock.s_inode_start;
    fseek(archivo, posLectura, SEEK_SET);
    fread(&linodo, sizeof(inodo), 1, archivo);

    //BUSCAR LA CARPETA A ELIMINAR
    posicion = 1;
    continuar = true;
    if(ruta == "/"){
        std::cout << "ERROR: No se puede borrar la raíz." << std::endl;
        fclose(archivo);
        return;
    }else if(path[0] != "\0"){
        std::cout << "ERROR: La ruta ingresada es erronea." << std::endl;
        fclose(archivo);
        return;
    }

    while(continuar){
        int inodo_temporal = -1;
        
        for(int i = 0; i < 15; i++){
            if(inodo_leido != -1 || inodo_temporal != -1){
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

                        if(carpeta == path[posicion]){
                            if(posicion == path.size() - 1){
                                inodo_leido = lcarpeta.b_content[k].b_inodo;
                                posicion += 1;
                                continuar = false;
                                break;
                            }else{
                                inodo_temporal = lcarpeta.b_content[k].b_inodo;
                                inodo_padre = inodo_temporal;
                                posicion += 1;
                                posLectura = sblock.s_inode_start + (sizeof(inodo) * inodo_temporal);
                                fseek(archivo, posLectura, SEEK_SET);
                                fread(&linodo, sizeof(inodo), 1, archivo);

                                if(linodo.i_type == '1'){
                                    continuar = false;
                                    break;
                                }
                            }  
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

                            if(carpeta == path[posicion]){
                                if(posicion == path.size() - 1){
                                    inodo_leido = lcarpeta.b_content[l].b_inodo;
                                    posicion += 1;
                                    continuar = false;
                                    break;
                                }else{
                                    inodo_temporal = lcarpeta.b_content[l].b_inodo;
                                    inodo_padre = inodo_temporal;
                                    posicion += 1;
                                    posLectura = sblock.s_inode_start + (sizeof(inodo) * inodo_temporal);
                                    fseek(archivo, posLectura, SEEK_SET);
                                    fread(&linodo, sizeof(inodo), 1, archivo);

                                    if(linodo.i_type == '1'){
                                        continuar = false;
                                        break;
                                    }
                                }
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

                                if(carpeta == path[posicion]){
                                    if(posicion == path.size() - 1){
                                        inodo_leido = lcarpeta.b_content[m].b_inodo;
                                        posicion += 1;
                                        continuar = false;
                                        break;
                                    }else{
                                        inodo_temporal = lcarpeta.b_content[m].b_inodo;
                                        inodo_padre = inodo_temporal;
                                        posicion += 1;
                                        posLectura = sblock.s_inode_start + (sizeof(inodo) * inodo_temporal);
                                        fseek(archivo, posLectura, SEEK_SET);
                                        fread(&linodo, sizeof(inodo), 1, archivo);

                                        if(linodo.i_type == '1'){
                                            continuar = false;
                                            break;
                                        }
                                    }
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
        
                    if(carpeta == path[posicion]){
                        if(posicion == path.size() - 1){
                            inodo_leido = lcarpeta.b_content[j].b_inodo;
                            posicion += 1;
                            continuar = false;
                            break;
                        }else{
                            inodo_temporal = lcarpeta.b_content[j].b_inodo;
                            inodo_padre = inodo_temporal;
                            posicion += 1;
                            posLectura = sblock.s_inode_start + (sizeof(inodo) * inodo_temporal);
                            fseek(archivo, posLectura, SEEK_SET);
                            fread(&linodo, sizeof(inodo), 1, archivo);

                            if(linodo.i_type == '1'){
                                continuar = false;
                                break;
                            }
                        }
                    }
                }
            }
        }

        if(inodo_temporal == -1 && posicion != path.size()){
            continuar = false;
            std::cout << "ERROR: La ruta ingresada no existe." << std::endl;
            fclose(archivo);
            return;
        }else if(posicion == path.size() && inodo_leido == -1){
            continuar = false;
            std::cout << "ERROR: La ruta es erronea." << std::endl;
            fclose(archivo);
            return;
        }
    }
    
    //LEER EL INODO BUSCADO 
    posLectura = sblock.s_inode_start + (sizeof(inodo) * inodo_leido);
    fseek(archivo, posLectura, SEEK_SET);
    fread(&linodo, sizeof(inodo), 1, archivo);

    //BORRAR EL ARCHIVO
    fclose(archivo);
    if(linodo.i_type == '0'){
        borrar_padre = carpetas(sesion, sblock, tempD.ruta, borrar_bloques, borrar_inodos, inodo_leido);
    }else{
        borrar_padre = archivos(sesion, sblock, tempD.ruta, borrar_bloques, inodo_leido);
    }

    //BORRAR DE LA CARPETA PADRE Y LIMPIAR LOS BLOQUES VACIOS
    archivo = fopen(tempD.ruta.c_str(), "r+b");
    if(borrar_padre){
        //Añadir el inodo a borrar
        borrar_inodos.push_back(inodo_leido);

        //Leer el inodo del padre
        posLectura = sblock.s_inode_start + (sizeof(inodo) * inodo_padre);
        fseek(archivo, posLectura, SEEK_SET);
        fread(&linodo, sizeof(inodo), 1, archivo);

        //Buscar el archivo y eliminarlo
        int inodo_temporal = -1;
        for(int i = 0; i < 15; i++){
            if(inodo_temporal != -1){
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

                        if(carpeta == nombre_archivo){
                            //Actualizar Inodo
                            linodo.i_mtime = time(NULL);
                            posLectura = sblock.s_inode_start + (sizeof(inodo) * inodo_padre);
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&linodo, sizeof(inodo), 1, archivo);

                            //Borrar de la carpeta
                            strcpy(lcarpeta.b_content[k].b_name, "-");
                            lcarpeta.b_content[k].b_inodo = -1;
                            posLectura = sblock.s_block_start + (sizeof(bcarpetas) * lapuntador.b_pointers[j]);
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&lcarpeta, sizeof(bcarpetas), 1, archivo);
                            break;
                        }
                    }

                    if(inodo_temporal != -1){
                        break;
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

                            if(carpeta == nombre_archivo){
                                //Actualizar Inodo
                                linodo.i_mtime = time(NULL);
                                posLectura = sblock.s_inode_start + (sizeof(inodo) * inodo_padre);
                                fseek(archivo, posLectura, SEEK_SET);
                                fwrite(&linodo, sizeof(inodo), 1, archivo);

                                //Borrar de la carpeta
                                strcpy(lcarpeta.b_content[l].b_name, "-");
                                lcarpeta.b_content[l].b_inodo = -1;
                                posLectura = sblock.s_block_start + (sizeof(bcarpetas) * lapuntador.b_pointers[k]);
                                fseek(archivo, posLectura, SEEK_SET);
                                fwrite(&lcarpeta, sizeof(bcarpetas), 1, archivo);
                                break;
                            }
                        }

                        if(inodo_temporal != -1){
                            break;
                        }
                    }

                    if(inodo_temporal != -1){
                        break;
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

                                if(carpeta == nombre_archivo){
                                    //Actualizar Inodo
                                    linodo.i_mtime = time(NULL);
                                    posLectura = sblock.s_inode_start + (sizeof(inodo) * inodo_padre);
                                    fseek(archivo, posLectura, SEEK_SET);
                                    fwrite(&linodo, sizeof(inodo), 1, archivo);

                                    //Borrar de la carpeta
                                    strcpy(lcarpeta.b_content[m].b_name, "-");
                                    lcarpeta.b_content[m].b_inodo = -1;
                                    posLectura = sblock.s_block_start + (sizeof(bcarpetas) * lapuntador.b_pointers[l]);
                                    fseek(archivo, posLectura, SEEK_SET);
                                    fwrite(&lcarpeta, sizeof(bcarpetas), 1, archivo);
                                    break;
                                }
                            }

                            if(inodo_temporal != -1){
                                break;
                            }
                        }

                        if(inodo_temporal != -1){
                            break;
                        }
                    }
                    
                    if(inodo_temporal != -1){
                        break;
                    }
                }
            }else{
                posLectura = sblock.s_block_start + (sizeof(bcarpetas) * linodo.i_block[i]);
                fseek(archivo, posLectura, SEEK_SET);
                fread(&lcarpeta, sizeof(bcarpetas), 1, archivo);

                for(int j = 0; j < 4; j++){
                    std::string carpeta(lcarpeta.b_content[j].b_name);

                    if(carpeta == nombre_archivo){
                        //Actualizar Inodo
                        linodo.i_mtime = time(NULL);
                        posLectura = sblock.s_inode_start + (sizeof(inodo) * inodo_padre);
                        fseek(archivo, posLectura, SEEK_SET);
                        fwrite(&linodo, sizeof(inodo), 1, archivo);

                        //Borrar de la carpeta
                        strcpy(lcarpeta.b_content[j].b_name, "-");
                        lcarpeta.b_content[j].b_inodo = -1;
                        posLectura = sblock.s_block_start + (sizeof(bcarpetas) * linodo.i_block[i]);
                        fseek(archivo, posLectura, SEEK_SET);
                        fwrite(&lcarpeta, sizeof(bcarpetas), 1, archivo);
                        break;
                    }
                }
            }
        }

        //Actualizar Inodo
        linodo.i_mtime = time(NULL);
        posLectura = sblock.s_inode_start + (sizeof(inodo) * inodo_padre);
        fseek(archivo, posLectura, SEEK_SET);
        fwrite(&linodo, sizeof(inodo), 1, archivo);

        //Limpiar bloques
        fclose(archivo);
        limpiar_carpeta(sblock, tempD.ruta, borrar_bloques, inodo_padre);
    }
    
    
    //BORRAR LOS BLOQUES EN EL BITMAP DE BLOQUES
    archivo = fopen(tempD.ruta.c_str(), "r+b");
    char s = '\0'; 
    sblock.s_free_blocks_count += borrar_bloques.size();
    for(int a = 0; a < borrar_bloques.size(); a++){
        posLectura = sblock.s_bm_block_start + (sizeof(char) * borrar_bloques[a]);
        fseek(archivo, posLectura, SEEK_SET);
        fwrite(&s, sizeof(char), 1, archivo);
    }

    //BORRAR LOS BLOQUES EN EL BITMAP DE INODOS 
    sblock.s_free_blocks_count += borrar_inodos.size();
    for(int a = 0; a < borrar_inodos.size(); a++){
        posLectura = sblock.s_bm_inode_start + (sizeof(char) * borrar_inodos[a]);
        fseek(archivo, posLectura, SEEK_SET);
        fwrite(&s, sizeof(char), 1, archivo);
    }

    //ACTUALIZAR EL SUPERBLOQUE
    fseek(archivo, posInicio, SEEK_SET);
    fwrite(&sblock, sizeof(sbloque), 1, archivo);
 
    //ESCRIBIR EN EL JOURNAL EL COMANDO
    //En contenido: id_usr, usr, id_grp, grp, disco
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
            std::string contenido = sesion.id_user;
            contenido.append(",");
            contenido.append(sesion.user);
            contenido.append(",");
            contenido.append(sesion.id_grp);
            contenido.append(",");
            contenido.append(sesion.grupo);
            contenido.append(",");
            contenido.append(sesion.disco);
            
            strcpy(creacion.comando ,"remove");
            strcpy(creacion.path ,ruta.c_str());
            strcpy(creacion.nombre, nombre_archivo.c_str());
            strcpy(creacion.contenido, contenido.c_str());
            creacion.fecha = time(NULL);
            fseek(archivo, posRegistro, SEEK_SET);
            fwrite(&creacion, sizeof(registro), 1, archivo);
        }
    }

    fclose(archivo);
    std::cout << "MENSAJE: Archivo/Carpeta borrado correctamente." << std::endl;
}

bool carpetas(usuario &sesion, sbloque &sblock, std::string &ruta, std::vector<int> &borrar_bloques, std::vector<int> &borrar_inodos, int &no_inodo){
    //VARIABLES
    FILE *archivo;                             //Sirve para verificar que el archivo exista
    int posLectura;                            //Para determinar la posicion de lectura en disco
    inodo linodo;                              //Para el manejo de los inodos
    inodo tinodo;                              //Permite saber si un inodo es de archivo o carpeta
    bcarpetas lcarpeta;                        //Para el manejo de bloques de carpetas
    barchivos larchivo;                        //Para el manejo de bloques de archivo
    bapuntadores lapuntador;                   //Para el manejo de bloques de apuntadores simples
    bapuntadores lapuntador_doble;             //Para el manejo de bloques de apuntadores dobles
    bapuntadores lapuntador_triple;            //Para el manejo de bloques de apuntadores triples
    bool continuar = true;                     //Usado como bandera al buscar la ruta

    //ABRIR EL ARCHIVO
    archivo = fopen(ruta.c_str(), "r+b");

    //LEER EL INODO
    posLectura = sblock.s_inode_start + (sizeof(inodo) * no_inodo);
    fseek(archivo, posLectura, SEEK_SET);
    fread(&linodo, sizeof(inodo), 1, archivo);

    //VERIFICAR QUE POSEA PERMISO PARA ESCRIBIR Y LEER 
    std::string permisos = std::to_string(linodo.i_perm);
    int ugo = 3;      //1 para dueño, 2 para grupo, 3 para otros
    bool acceso = false;

    if(stoi(sesion.id_user) == linodo.i_uid){
        ugo = 1;
    }else if(stoi(sesion.id_grp) == linodo.i_gid){
        ugo = 2;
    }

    if(ugo == 1){
        if(permisos[0] == '2' || permisos[0] == '3' || permisos[0] == '6' || permisos[0] == '7'){
            acceso = true;
        }
    }else if(ugo == 2){
        if(permisos[1] == '2' || permisos[1] == '3' || permisos[1] == '6' || permisos[1] == '7'){
            acceso = true;
        }
    }else{
        if(permisos[2] == '2' || permisos[2] == '3' || permisos[2] == '6' || permisos[2] == '7'){
            acceso = true;
        }
    }

    if(sesion.user == "root"){
        acceso = true;
    }

    if(!acceso){
        std::cout << "ERROR: No posee permisos para eliminar archivos en esta carpeta." << std::endl;
        fclose(archivo);
        return false;
    }

    //RECORRER LA CARPETA
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

                posLectura = sblock.s_block_start + (sizeof(bcarpetas) * lapuntador.b_pointers[j]);
                fseek(archivo, posLectura, SEEK_SET);
                fread(&lcarpeta, sizeof(bcarpetas), 1, archivo);

                for(int k = 0; k < 4; k++){
                    std::string carpeta(lcarpeta.b_content[k].b_name);

                    if(carpeta != "-"){
                        bool borrar;
                        //Determinar el tipo       
                        posLectura = sblock.s_inode_start + (sizeof(inodo) * lcarpeta.b_content[k].b_inodo);
                        fseek(archivo, posLectura, SEEK_SET);
                        fread(&tinodo, sizeof(inodo), 1, archivo);

                        fclose(archivo);
                        if(tinodo.i_type == '0'){
                            borrar = carpetas(sesion, sblock, ruta, borrar_bloques, borrar_inodos, lcarpeta.b_content[k].b_inodo);
                        }else{
                            borrar = archivos(sesion, sblock, ruta, borrar_bloques, lcarpeta.b_content[k].b_inodo);
                        }

                        //Borrar la carpeta y actualizar el bloque
                        archivo = fopen(ruta.c_str(), "r+b");
                        if(borrar){
                            borrar_inodos.push_back(lcarpeta.b_content[k].b_inodo);
                            strcpy(lcarpeta.b_content[k].b_name, "-");
                            lcarpeta.b_content[k].b_inodo = -1;
                            posLectura = sblock.s_block_start + (sizeof(bcarpetas) * lapuntador.b_pointers[j]);
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&lcarpeta, sizeof(bcarpetas), 1, archivo);   
                        }
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

                        if(carpeta != "-"){
                            bool borrar;
                            //Determinar el tipo       
                            posLectura = sblock.s_inode_start + (sizeof(inodo) * lcarpeta.b_content[l].b_inodo);
                            fseek(archivo, posLectura, SEEK_SET);
                            fread(&tinodo, sizeof(inodo), 1, archivo);

                            fclose(archivo);
                            if(tinodo.i_type == '0'){
                                borrar = carpetas(sesion, sblock, ruta, borrar_bloques, borrar_inodos, lcarpeta.b_content[l].b_inodo);
                            }else{
                                borrar = archivos(sesion, sblock, ruta, borrar_bloques, lcarpeta.b_content[l].b_inodo);
                            }

                            //Borrar la carpeta y actualizar el bloque
                            archivo = fopen(ruta.c_str(), "r+b");
                            if(borrar){
                                borrar_inodos.push_back(lcarpeta.b_content[l].b_inodo);
                                strcpy(lcarpeta.b_content[l].b_name, "-");
                                lcarpeta.b_content[l].b_inodo = -1;
                                posLectura = sblock.s_block_start + (sizeof(bcarpetas) * lapuntador.b_pointers[k]);
                                fseek(archivo, posLectura, SEEK_SET);
                                fwrite(&lcarpeta, sizeof(bcarpetas), 1, archivo);   
                            }
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

                            if(carpeta == "-"){
                                bool borrar;
                                //Determinar el tipo       
                                posLectura = sblock.s_inode_start + (sizeof(inodo) * lcarpeta.b_content[m].b_inodo);
                                fseek(archivo, posLectura, SEEK_SET);
                                fread(&tinodo, sizeof(inodo), 1, archivo);

                                fclose(archivo);
                                if(tinodo.i_type == '0'){
                                    borrar = carpetas(sesion, sblock, ruta, borrar_bloques, borrar_inodos, lcarpeta.b_content[m].b_inodo);
                                }else{
                                    borrar = archivos(sesion, sblock, ruta, borrar_bloques, lcarpeta.b_content[m].b_inodo);
                                }

                                //Borrar la carpeta y actualizar el bloque
                                archivo = fopen(ruta.c_str(), "r+b");
                                if(borrar){
                                    borrar_inodos.push_back(lcarpeta.b_content[m].b_inodo);
                                    strcpy(lcarpeta.b_content[m].b_name, "-");
                                    lcarpeta.b_content[m].b_inodo = -1;
                                    posLectura = sblock.s_block_start + (sizeof(bcarpetas) * lapuntador.b_pointers[l]);
                                    fseek(archivo, posLectura, SEEK_SET);
                                    fwrite(&lcarpeta, sizeof(bcarpetas), 1, archivo);   
                                }
                            }
                        }
                    }
                }
            }

        }else if(i == 0){
            posLectura = sblock.s_block_start + (sizeof(bcarpetas) * linodo.i_block[i]);
            fseek(archivo, posLectura, SEEK_SET);
            fread(&lcarpeta, sizeof(bcarpetas), 1, archivo);

            //Revisar espacios
            for(int j = 2; j < 4; j++){
                std::string carpeta(lcarpeta.b_content[j].b_name);

                if(carpeta != "-"){
                    bool borrar;
                    //Determinar el tipo       
                    posLectura = sblock.s_inode_start + (sizeof(inodo) * lcarpeta.b_content[j].b_inodo);
                    fseek(archivo, posLectura, SEEK_SET);
                    fread(&tinodo, sizeof(inodo), 1, archivo);

                    fclose(archivo);
                    if(tinodo.i_type == '0'){
                        borrar = carpetas(sesion, sblock, ruta, borrar_bloques, borrar_inodos, lcarpeta.b_content[j].b_inodo);
                    }else{
                        borrar = archivos(sesion, sblock, ruta, borrar_bloques, lcarpeta.b_content[j].b_inodo);
                    }

                    //Borrar la carpeta y actualizar el bloque
                    archivo = fopen(ruta.c_str(), "r+b");
                    if(borrar){
                        borrar_inodos.push_back(lcarpeta.b_content[j].b_inodo);
                        strcpy(lcarpeta.b_content[j].b_name, "-");
                        lcarpeta.b_content[j].b_inodo = -1;
                        posLectura = sblock.s_block_start + (sizeof(bcarpetas) * linodo.i_block[i]);
                        fseek(archivo, posLectura, SEEK_SET);
                        fwrite(&lcarpeta, sizeof(bcarpetas), 1, archivo);   
                    }
                }
            }
        }
        else{
            posLectura = sblock.s_block_start + (sizeof(bcarpetas) * linodo.i_block[i]);
            fseek(archivo, posLectura, SEEK_SET);
            fread(&lcarpeta, sizeof(bcarpetas), 1, archivo);

            //Revisar espacios
            for(int j = 0; j < 4; j++){
                std::string carpeta(lcarpeta.b_content[j].b_name);

                if(carpeta != "-"){
                    bool borrar;
                    //Determinar el tipo       
                    posLectura = sblock.s_inode_start + (sizeof(inodo) * lcarpeta.b_content[j].b_inodo);
                    fseek(archivo, posLectura, SEEK_SET);
                    fread(&tinodo, sizeof(inodo), 1, archivo);

                    fclose(archivo);
                    if(tinodo.i_type == '0'){
                        borrar = carpetas(sesion, sblock, ruta, borrar_bloques, borrar_inodos, lcarpeta.b_content[j].b_inodo);
                    }else{
                        borrar = archivos(sesion, sblock, ruta, borrar_bloques, lcarpeta.b_content[j].b_inodo);
                    }

                    //Borrar la carpeta y actualizar el bloque
                    archivo = fopen(ruta.c_str(), "r+b");
                    if(borrar){
                        borrar_inodos.push_back(lcarpeta.b_content[j].b_inodo);
                        strcpy(lcarpeta.b_content[j].b_name, "-");
                        lcarpeta.b_content[j].b_inodo = -1;
                        posLectura = sblock.s_block_start + (sizeof(bcarpetas) * linodo.i_block[i]);
                        fseek(archivo, posLectura, SEEK_SET);
                        fwrite(&lcarpeta, sizeof(bcarpetas), 1, archivo);   
                    }
                }
            }
        }
    }
    
    //LIMPIAR EL INODO
    fclose(archivo);
    limpiar_carpeta(sblock, ruta, borrar_bloques, no_inodo);

    //LEER EL INODO ACTUALIZADO
    archivo = fopen(ruta.c_str(), "r+b");
    posLectura = sblock.s_inode_start + (sizeof(inodo) * no_inodo);
    fseek(archivo, posLectura, SEEK_SET);
    fread(&linodo, sizeof(inodo), 1, archivo);
    fclose(archivo);

    //DETERMINAR SI SE VA A BORRAR EL INODO
    int cinodo = 0;
    for(int i = 0; i < 15; i++){
        if(linodo.i_block[i] == -1){
            cinodo += 1;
        }
    }

    if(cinodo == 15){
        return true;
    }else{
        return false;
    }    
}

bool archivos(usuario &sesion, sbloque &sblock, std::string &ruta, std::vector<int> &borrar_bloques, int &no_inodo){
    //VARIABLES
    FILE *archivo;                             //Sirve para verificar que el archivo exista
    int posLectura;                            //Para determinar la posicion de lectura en disco
    inodo linodo;                              //Para el manejo de los inodos
    bcarpetas lcarpeta;                        //Para el manejo de bloques de carpetas
    barchivos larchivo;                        //Para el manejo de bloques de archivo
    bapuntadores lapuntador;                   //Para el manejo de bloques de apuntadores simples
    bapuntadores lapuntador_doble;             //Para el manejo de bloques de apuntadores dobles
    bapuntadores lapuntador_triple;            //Para el manejo de bloques de apuntadores triples
    bool continuar = true;                     //Usado como bandera al buscar la ruta

    //ABRIR EL ARCHIVO
    archivo = fopen(ruta.c_str(), "r+b");

    //LEER EL INODO
    posLectura = sblock.s_inode_start + (sizeof(inodo) * no_inodo);
    fseek(archivo, posLectura, SEEK_SET);
    fread(&linodo, sizeof(inodo), 1, archivo);

    //VERIFICAR QUE POSEA PERMISO PARA ESCRIBIR Y LEER 
    std::string permisos = std::to_string(linodo.i_perm);
    int ugo = 3;      //1 para dueño, 2 para grupo, 3 para otros
    bool acceso = false;

    if(stoi(sesion.id_user) == linodo.i_uid){
        ugo = 1;
    }else if(stoi(sesion.id_grp) == linodo.i_gid){
        ugo = 2;
    }

    if(ugo == 1){
        if(permisos[0] == '2' || permisos[0] == '3' || permisos[0] == '6' || permisos[0] == '7'){
            acceso = true;
        }
    }else if(ugo == 2){
        if(permisos[1] == '2' || permisos[1] == '3' || permisos[1] == '6' || permisos[1] == '7'){
            acceso = true;
        }
    }else{
        if(permisos[2] == '2' || permisos[2] == '3' || permisos[2] == '6' || permisos[2] == '7'){
            acceso = true;
        }
    }

    if(sesion.user == "root"){
        acceso = true;
    }

    if(!acceso){
        std::cout << "ERROR: No posee permisos para eliminar el archivo." << std::endl;
        fclose(archivo);
        return false;
    }

    //BUSCAR LOS BLOQUES QUE SE VAN A BORRAR
    for(int i = 0; i < 15; i++){
        
        if(linodo.i_block[i] == -1){
            continue;
        }

        borrar_bloques.push_back(linodo.i_block[i]);

        if(i == 12){
            //Recorrer el bloque de apuntadores simple
            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * linodo.i_block[i]);
            fseek(archivo, posLectura, SEEK_SET);
            fread(&lapuntador, sizeof(bapuntadores), 1, archivo);

            for(int j = 0; j < 16; j++){
                if(lapuntador.b_pointers[j] == -1){
                    continue;
                }

                borrar_bloques.push_back(lapuntador.b_pointers[j]);
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

                borrar_bloques.push_back(lapuntador_doble.b_pointers[j]);

                posLectura = sblock.s_block_start + (sizeof(bapuntadores) * lapuntador_doble.b_pointers[j]);
                fseek(archivo, posLectura, SEEK_SET);
                fread(&lapuntador, sizeof(bapuntadores), 1, archivo);

                for(int k = 0; k < 16; k++){
                    if(lapuntador.b_pointers[k] == -1){
                        continue;
                    }

                    borrar_bloques.push_back(lapuntador.b_pointers[k]);
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

                borrar_bloques.push_back(lapuntador_triple.b_pointers[j]);

                posLectura = sblock.s_block_start + (sizeof(bapuntadores) * lapuntador_triple.b_pointers[j]);
                fseek(archivo, posLectura, SEEK_SET);
                fread(&lapuntador_doble, sizeof(bapuntadores), 1, archivo);

                for(int k = 0; k < 16; k++){
                    if(lapuntador_doble.b_pointers[k] == -1){
                        continue;
                    }

                    borrar_bloques.push_back(lapuntador_doble.b_pointers[k]);

                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * lapuntador_doble.b_pointers[k]);
                    fseek(archivo, posLectura, SEEK_SET);
                    fread(&lapuntador, sizeof(bapuntadores), 1, archivo);

                    for(int l = 0; l < 16; l++){
                        if(lapuntador.b_pointers[l] == -1){
                            continue;
                        }

                        borrar_bloques.push_back(lapuntador.b_pointers[l]);
                    }
                }
            }
        }
    }

    fclose(archivo);
    return true;

}

void limpiar_carpeta(sbloque &sblock, std::string &ruta, std::vector<int> &borrar_bloques, int &no_inodo){
    //VARIABLES
    FILE *archivo;                             //Sirve para verificar que el archivo exista
    int posLectura;                            //Para determinar la posicion de lectura en disco
    inodo linodo;                              //Para el manejo de los inodos
    bcarpetas lcarpeta;                        //Para el manejo de bloques de carpetas
    barchivos larchivo;                        //Para el manejo de bloques de archivo
    bapuntadores lapuntador;                   //Para el manejo de bloques de apuntadores simples
    bapuntadores lapuntador_doble;             //Para el manejo de bloques de apuntadores dobles
    bapuntadores lapuntador_triple;            //Para el manejo de bloques de apuntadores triples
    bool continuar = true;                     //Usado como bandera al buscar la ruta

    //ABRIR EL ARCHIVO
    archivo = fopen(ruta.c_str(), "r+b");

    //LEER EL INODO
    posLectura = sblock.s_inode_start + (sizeof(inodo) * no_inodo);
    fseek(archivo, posLectura, SEEK_SET);
    fread(&linodo, sizeof(inodo), 1, archivo);

    //REVISAR EL INODO Y BORRAR LOS BLOQUES QE NO SE ESTEN USANDO
    for(int i = 0; i < 15; i++){
        int ccarpetas = 0;

        if(linodo.i_block[i] == -1){
            continue;
        }
        
        if(i == 12){
            //Recorrer el bloque de apuntadores simple
            int csimples = 0;
            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * linodo.i_block[i]);
            fseek(archivo, posLectura, SEEK_SET);
            fread(&lapuntador, sizeof(bapuntadores), 1, archivo);

            for(int j = 0; j < 16; j++){
                if(lapuntador.b_pointers[j] == -1){
                    csimples += 1;
                    continue;
                }

                posLectura = sblock.s_block_start + (sizeof(bcarpetas) * lapuntador.b_pointers[j]);
                fseek(archivo, posLectura, SEEK_SET);
                fread(&lcarpeta, sizeof(bcarpetas), 1, archivo);

                for(int k = 0; k < 4; k++){
                    std::string carpeta(lcarpeta.b_content[k].b_name);

                    if(carpeta == "-"){
                        ccarpetas += 1;
                    }
                }

                if(ccarpetas == 4){
                    borrar_bloques.push_back(lapuntador.b_pointers[j]);
                    lapuntador.b_pointers[j] = -1;
                    csimples += 1;
                }
            }

            if(csimples == 16){
                borrar_bloques.push_back(linodo.i_block[i]);
                linodo.i_block[i] = -1;
            }

            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * linodo.i_block[i]);
            fseek(archivo, posLectura, SEEK_SET);
            fwrite(&lapuntador, sizeof(bapuntadores), 1, archivo);

        }else if(i == 13){
            //Recorrer el bloque de apuntadores doble
            int cdobles = 0;
            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * linodo.i_block[i]);
            fseek(archivo, posLectura, SEEK_SET);
            fread(&lapuntador_doble, sizeof(bapuntadores), 1, archivo);

            for(int j = 0; j < 16; j++){
                if(lapuntador_doble.b_pointers[j] == -1){
                    cdobles += 1;
                    continue;
                }

                int csimples = 0;
                posLectura = sblock.s_block_start + (sizeof(bapuntadores) * lapuntador_doble.b_pointers[j]);
                fseek(archivo, posLectura, SEEK_SET);
                fread(&lapuntador, sizeof(bapuntadores), 1, archivo);

                for(int k = 0; k < 16; k++){
                    if(lapuntador.b_pointers[k] == -1){
                        csimples += 1;
                        continue;
                    }

                    posLectura = sblock.s_block_start + (sizeof(bcarpetas) * lapuntador.b_pointers[k]);
                    fseek(archivo, posLectura, SEEK_SET);
                    fread(&lcarpeta, sizeof(bcarpetas), 1, archivo);

                    for(int l = 0; l < 4; l++){
                        std::string carpeta(lcarpeta.b_content[l].b_name);

                        if(carpeta == "-"){
                            ccarpetas += 1;
                        }
                    }

                    if(ccarpetas == 4){
                        borrar_bloques.push_back(lapuntador.b_pointers[k]);
                        lapuntador.b_pointers[k] = -1;
                        csimples += 1;
                    }
                }

                if(csimples == 16){
                    borrar_bloques.push_back(lapuntador_doble.b_pointers[j]);
                    lapuntador_doble.b_pointers[j] = -1;
                    cdobles += 1;
                }

                posLectura = sblock.s_block_start + (sizeof(bapuntadores) * lapuntador_doble.b_pointers[j]);
                fseek(archivo, posLectura, SEEK_SET);
                fwrite(&lapuntador, sizeof(bapuntadores), 1, archivo);
            }

            if(cdobles == 16){
                borrar_bloques.push_back(linodo.i_block[i]);
                linodo.i_block[i] = -1;
            }

            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * linodo.i_block[i]);
            fseek(archivo, posLectura, SEEK_SET);
            fwrite(&lapuntador_doble, sizeof(bapuntadores), 1, archivo);

        }else if(i == 14){
            //Recorrer el bloque de apuntadores triple
            int ctriples = 0;
            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * linodo.i_block[i]);
            fseek(archivo, posLectura, SEEK_SET);
            fread(&lapuntador_triple, sizeof(bapuntadores), 1, archivo);

            for(int j = 0; j < 16; j++){
                if(lapuntador_triple.b_pointers[j] == -1){
                    ctriples += 1;
                    continue;
                }

                int cdobles = 0;
                posLectura = sblock.s_block_start + (sizeof(bapuntadores) * lapuntador_triple.b_pointers[j]);
                fseek(archivo, posLectura, SEEK_SET);
                fread(&lapuntador_doble, sizeof(bapuntadores), 1, archivo);

                for(int k = 0; k < 16; k++){
                    if(lapuntador_doble.b_pointers[k] == -1){
                        cdobles += 1;
                        continue;
                    }
                    
                    int csimples = 0;
                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * lapuntador_doble.b_pointers[k]);
                    fseek(archivo, posLectura, SEEK_SET);
                    fread(&lapuntador, sizeof(bapuntadores), 1, archivo);

                    for(int l = 0; l < 16; l++){
                        if(lapuntador.b_pointers[l] == -1){
                            csimples += 1;
                            continue;
                        }
                        
                        posLectura = sblock.s_block_start + (sizeof(bcarpetas) * lapuntador.b_pointers[l]);
                        fseek(archivo, posLectura, SEEK_SET);
                        fread(&lcarpeta, sizeof(bcarpetas), 1, archivo);

                        for(int m = 0; m < 4; m++){
                            std::string carpeta(lcarpeta.b_content[m].b_name);

                            if(carpeta == "-"){
                                ccarpetas += 1;
                            }
                        }

                        if(ccarpetas == 4){
                            borrar_bloques.push_back(lapuntador.b_pointers[l]);
                            lapuntador.b_pointers[l] = -1;
                            csimples += 1;
                        }
                    }

                    if(csimples == 16){
                        borrar_bloques.push_back(lapuntador_doble.b_pointers[k]);
                        lapuntador_doble.b_pointers[k] = -1;
                        cdobles += 1;
                    }

                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * lapuntador_doble.b_pointers[k]);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&lapuntador, sizeof(bapuntadores), 1, archivo);
                }
                
                if(cdobles == 16){
                    borrar_bloques.push_back(lapuntador_triple.b_pointers[j]);
                    lapuntador_triple.b_pointers[j] = -1;
                    ctriples += 1;
                }

                posLectura = sblock.s_block_start + (sizeof(bapuntadores) * lapuntador_triple.b_pointers[j]);
                fseek(archivo, posLectura, SEEK_SET);
                fwrite(&lapuntador_doble, sizeof(bapuntadores), 1, archivo);
            }

            if(ctriples == 16){
                borrar_bloques.push_back(linodo.i_block[i]);
                linodo.i_block[i] = -1;
            }

            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * linodo.i_block[i]);
            fseek(archivo, posLectura, SEEK_SET);
            fwrite(&lapuntador_triple, sizeof(bapuntadores), 1, archivo);

        }else if(i == 0){
            
        }else{
            posLectura = sblock.s_block_start + (sizeof(bcarpetas) * linodo.i_block[i]);
            fseek(archivo, posLectura, SEEK_SET);
            fread(&lcarpeta, sizeof(bcarpetas), 1, archivo);

            //Revisar espacios
            for(int j = 0; j < 4; j++){
                std::string carpeta(lcarpeta.b_content[j].b_name);

                if(carpeta == "-"){
                    ccarpetas += 1;
                }
            }

            if(ccarpetas == 4){
                borrar_bloques.push_back(linodo.i_block[i]);
                linodo.i_block[i] = -1;
            }
        }
    }
    
    //ACTUALIZAR EL INODO
    posLectura = sblock.s_inode_start + (sizeof(inodo) * no_inodo);
    fseek(archivo, posLectura, SEEK_SET);
    fwrite(&linodo, sizeof(inodo), 1, archivo);
    fclose(archivo);
}
