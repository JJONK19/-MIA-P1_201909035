#include "mkdir.h"

void mkdir(std::vector<std::string> &parametros, std::vector<disco> &discos, usuario &sesion){
    
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
    bool padre = false;                        //Atributo p
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
    int inodo_leido = -1;                      //Inodo actual con el que se este trabajando
    bool error_permisos = false;               //Bandera para indicar que no se pudo crear en una carpeta por su padre
    bool error_padre = false;                  //Indica que una carpeta no existe y no se pudo crear

    //COMPROBACIÓN DE PARAMETROS
    for(int i = 1; i < parametros.size(); i++){
        std::string &temp = parametros[i];
        std::vector<std::string> salida(std::sregex_token_iterator(temp.begin(), temp.end(), flecha, -1),
                    std::sregex_token_iterator());

        //Se separa en dos para manejar el atributo -r
        if(salida.size() == 1){
            std::string &tag = salida[0];
            
            //Pasar a minusculas
            transform(tag.begin(), tag.end(), tag.begin(),[](unsigned char c){
                return tolower(c);
            });

            if(tag == "p"){
                padre = true;
            }else{
                std::cout << "ERROR: El parametro " << tag << " no es valido." << std::endl;
                paramFlag = false;
                break;
            }
            
        }else{
            std::string &tag = salida[0];
            std::string &value = salida[1];

            //Pasar a minusculas
            transform(tag.begin(), tag.end(), tag.begin(),[](unsigned char c){
                return tolower(c);
            });

            if(tag == "path"){
                ruta = value;
            }else if(tag == "p"){
                std::cout << "ERROR: El parametro 'p' no recibe ningún valor." << std::endl;
                paramFlag = false;
                break;
            }else{
                std::cout << "ERROR: El parametro " << tag << " no es valido." << std::endl;
                paramFlag = false;
                break;
            }
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
        std::cout << "ERROR: La instrucción mkdir carece de todos los parametros obligatorios." << std::endl;
    }

    //PREPARACIÓN DE PARAMETROS - Separar los nombres que vengan en la ruta.
    std::regex separador("/");
    std::vector<std::string> path(std::sregex_token_iterator(ruta.begin(), ruta.end(), separador, -1),
                    std::sregex_token_iterator());

    //PREPARACIÓN DE PARAMETROS - Reducir el tamaño de los nombres al limite
    for(int i = 0; i < path.size(); i++){
        if(path[i].size() >= 12){
            path[i] = path[i].substr(0, 11);
        }
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
    inodo_leido = 0;
    fseek(archivo, posLectura, SEEK_SET);
    fread(&linodo, sizeof(inodo), 1, archivo);

    //CREAR LAS CARPETAS QUE HAGAN FALTA EN LA RUTA 
    posicion = 1;
    if(ruta == "/"){
        continuar = false;
    }else if(path[0] != "\0"){
        continuar = false;
    }
    
    while(continuar){
        bool crear = false;
        int inodo_temporal = -1;
        
        //1. Buscar si existe la carpeta
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

                        if(carpeta == path[posicion]){

                            //Actualizar Inodo
                            linodo.i_atime = time(NULL);
                            posLectura = sblock.s_inode_start + (sizeof(inodo) * inodo_leido);
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&linodo, sizeof(inodo), 1, archivo);
                    
                            inodo_temporal = lcarpeta.b_content[k].b_inodo;
                            inodo_leido = inodo_temporal;
                            posicion += 1;
                            posLectura = sblock.s_inode_start + (sizeof(inodo) * inodo_temporal);
                            fseek(archivo, posLectura, SEEK_SET);
                            fread(&linodo, sizeof(inodo), 1, archivo);
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

                            if(carpeta == path[posicion]){
                                //Actualizar Inodo
                                linodo.i_atime = time(NULL);
                                posLectura = sblock.s_inode_start + (sizeof(inodo) * inodo_leido);
                                fseek(archivo, posLectura, SEEK_SET);
                                fwrite(&linodo, sizeof(inodo), 1, archivo);
                        
                                inodo_temporal = lcarpeta.b_content[l].b_inodo;
                                inodo_leido = inodo_temporal;
                                posicion += 1;
                                posLectura = sblock.s_inode_start + (sizeof(inodo) * inodo_temporal);
                                fseek(archivo, posLectura, SEEK_SET);
                                fread(&linodo, sizeof(inodo), 1, archivo);
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

                                if(carpeta == path[posicion]){
                                    //Actualizar Inodo
                                    linodo.i_atime = time(NULL);
                                    posLectura = sblock.s_inode_start + (sizeof(inodo) * inodo_leido);
                                    fseek(archivo, posLectura, SEEK_SET);
                                    fwrite(&linodo, sizeof(inodo), 1, archivo);
                            
                                    inodo_temporal = lcarpeta.b_content[m].b_inodo;
                                    inodo_leido = inodo_temporal;
                                    posicion += 1;
                                    posLectura = sblock.s_inode_start + (sizeof(inodo) * inodo_temporal);
                                    fseek(archivo, posLectura, SEEK_SET);
                                    fread(&linodo, sizeof(inodo), 1, archivo);
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

                    if(carpeta == path[posicion]){
                        linodo.i_atime = time(NULL);
                        posLectura = sblock.s_inode_start + (sizeof(inodo) * inodo_leido);
                        fseek(archivo, posLectura, SEEK_SET);
                        fwrite(&linodo, sizeof(inodo), 1, archivo);
                        
                        inodo_temporal = lcarpeta.b_content[j].b_inodo;
                        inodo_leido = inodo_temporal;
                        posicion += 1;
                        posLectura = sblock.s_inode_start + (sizeof(inodo) * inodo_temporal);
                        fseek(archivo, posLectura, SEEK_SET);
                        fread(&linodo, sizeof(inodo), 1, archivo);
                        break;
                    }
                }
            }
        }
        
        //Determinar si se puede crear
        if(inodo_temporal == -1){
            //Revisar que tenga permisos sobre la carpeta
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

            if(acceso){
                if(padre){
                    crear = true;
                }else if(posicion == path.size() - 1){
                    crear = true;
                }else{
                    error_padre = true;
                }
            }else{
                continuar = false;
                error_permisos = true;
            }
        }

        //2. Crear la carpeta si no existe
        if(crear){
            bool buscar = true;
            int bloque_usado = -1;
            int bloque_intermedio = -1;
            int bloque_apuntadors = -1;
            int bloque_apuntadord = -1;
            int bloque_apuntadort = -1;
            inodo cinodo;
            bcarpetas ccarpeta;
            bapuntadores capuntador_simple;
            bapuntadores capuntador_doble;
            bapuntadores capuntador_triple;
            char c;

            for(int z = 0; z < 16; z++){
                capuntador_simple.b_pointers[z] = -1;
                capuntador_doble.b_pointers[z] = -1;
                capuntador_triple.b_pointers[z] = -1;
            }

            for(int z = 0; z < 4; z++){
                strcpy(ccarpeta.b_content[z].b_name, "-");
                ccarpeta.b_content[z].b_inodo = -1;
            }

            //Buscar un espacio en los bloques directos
            for(int i = 0; i < 12; i++){

                if(inodo_temporal != -1){
                    break;
                }

                if(linodo.i_block[i] != -1){
                    posLectura = sblock.s_block_start + (sizeof(bcarpetas) * linodo.i_block[i]);
                    fseek(archivo, posLectura, SEEK_SET);
                    fread(&lcarpeta, sizeof(bcarpetas), 1, archivo);

                    for(int j = 0; j < 4; j++){
                        std::string carpeta(lcarpeta.b_content[j].b_name);
                        
                        if(carpeta == "-"){
                            buscar = false;

                            for(int a = 0; a < sblock.s_inodes_count; a++){
                                posLectura = sblock.s_bm_inode_start + (sizeof(char) * a);
                                fseek(archivo, posLectura, SEEK_SET);
                                fread(&c, sizeof(char), 1, archivo);

                                if(c == '\0'){
                                    inodo_temporal = a;
                                    c = '1';
                                    fseek(archivo, posLectura, SEEK_SET);
                                    fwrite(&c ,sizeof(char), 1 ,archivo);
                                    break;
                                }

                                if(a == sblock.s_inodes_count - 1){
                                    std::cout << "ERROR: No hay inodos disponibles." << std::endl;
                                    fclose(archivo);
                                    return;
                                }
                            }

                            for(int a = 0; a < sblock.s_blocks_count; a++){
                                posLectura = sblock.s_bm_block_start + (sizeof(char) * a);
                                fseek(archivo, posLectura, SEEK_SET);
                                fread(&c, sizeof(char), 1, archivo);

                                if(c == '\0'){
                                    bloque_usado = a;
                                    c = 'c';
                                    fseek(archivo, posLectura, SEEK_SET);
                                    fwrite(&c ,sizeof(char), 1 ,archivo);
                                    break;
                                }

                                if(a == sblock.s_blocks_count - 1){
                                    std::cout << "ERROR: No hay bloques disponibles." << std::endl;
                                    fclose(archivo);
                                    return;
                                }
                            }

                            strcpy(lcarpeta.b_content[j].b_name, path[posicion].c_str());
                            lcarpeta.b_content[j].b_inodo = inodo_temporal;
                            posLectura = sblock.s_block_start + (sizeof(bcarpetas) * linodo.i_block[i]);
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&lcarpeta, sizeof(bcarpetas), 1, archivo);

                            //Actualizar el superbloque
                            sblock.s_free_blocks_count -= 1;
                            sblock.s_free_inodes_count -= 1;
                            break;
                        }
                    }
                }else{
                    buscar = false;
                    for(int a = 0; a < sblock.s_inodes_count; a++){
                        posLectura = sblock.s_bm_inode_start + (sizeof(char) * a);
                        fseek(archivo, posLectura, SEEK_SET);
                        fread(&c, sizeof(char), 1, archivo);

                        if(c == '\0'){
                            inodo_temporal = a;
                            c = '1';
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&c ,sizeof(char), 1 ,archivo);
                            break;
                        }

                        if(a == sblock.s_inodes_count - 1){
                            std::cout << "ERROR: No hay inodos disponibles." << std::endl;
                            fclose(archivo);
                            return;
                        }
                    }

                    for(int a = 0; a < sblock.s_blocks_count; a++){
                        posLectura = sblock.s_bm_block_start + (sizeof(char) * a);
                        fseek(archivo, posLectura, SEEK_SET);
                        fread(&c, sizeof(char), 1, archivo);

                        if(c == '\0'){
                            bloque_intermedio = a;
                            c = 'c';
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&c ,sizeof(char), 1 ,archivo);
                            break;
                        }

                        if(a == sblock.s_blocks_count - 1){
                            std::cout << "ERROR: No hay bloques disponibles." << std::endl;
                            fclose(archivo);
                            return;
                        }
                    }

                    for(int a = 0; a < sblock.s_blocks_count; a++){
                        posLectura = sblock.s_bm_block_start + (sizeof(char) * a);
                        fseek(archivo, posLectura, SEEK_SET);
                        fread(&c, sizeof(char), 1, archivo);

                        if(c == '\0'){
                            bloque_usado = a;
                            c = 'c';
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&c ,sizeof(char), 1 ,archivo);
                            break;
                        }

                        if(a == sblock.s_blocks_count - 1){
                            std::cout << "ERROR: No hay bloques disponibles." << std::endl;
                            fclose(archivo);
                            return;
                        }
                    }

                    //Escribir el nuevo bloque de carpeta
                    strcpy(ccarpeta.b_content[0].b_name, path[posicion].c_str());
                    ccarpeta.b_content[0].b_inodo = inodo_temporal;
                    posLectura = sblock.s_block_start + (sizeof(bcarpetas) * bloque_intermedio);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&ccarpeta, sizeof(bcarpetas), 1, archivo);

                    //Actualizar el inodo
                    linodo.i_block[i] = bloque_intermedio;
                    linodo.i_mtime = time(NULL);
                    posLectura = sblock.s_inode_start + (sizeof(inodo) * inodo_leido);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&linodo, sizeof(inodo), 1, archivo);

                    //Actualizar el superbloque
                    sblock.s_free_blocks_count -= 2;
                    sblock.s_free_inodes_count -= 1;
                    break;
                }
            }   

            //Buscar en los indirectos simples
            if(buscar){
                if(linodo.i_block[12] == -1){
                    buscar = false;
                    for(int a = 0; a < sblock.s_inodes_count; a++){
                        posLectura = sblock.s_bm_inode_start + (sizeof(char) * a);
                        fseek(archivo, posLectura, SEEK_SET);
                        fread(&c, sizeof(char), 1, archivo);

                        if(c == '\0'){
                            inodo_temporal = a;
                            c = '1';
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&c ,sizeof(char), 1 ,archivo);
                            break;
                        }

                        if(a == sblock.s_inodes_count - 1){
                            std::cout << "ERROR: No hay inodos disponibles." << std::endl;
                            fclose(archivo);
                            return;
                        }
                    }

                    for(int a = 0; a < sblock.s_blocks_count; a++){
                        posLectura = sblock.s_bm_block_start + (sizeof(char) * a);
                        fseek(archivo, posLectura, SEEK_SET);
                        fread(&c, sizeof(char), 1, archivo);

                        if(c == '\0'){
                            bloque_apuntadors = a;
                            c = 'p';
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&c ,sizeof(char), 1 ,archivo);
                            break;
                        }

                        if(a == sblock.s_blocks_count - 1){
                            std::cout << "ERROR: No hay bloques disponibles." << std::endl;
                            fclose(archivo);
                            return;
                        }
                    }

                    for(int a = 0; a < sblock.s_blocks_count; a++){
                        posLectura = sblock.s_bm_block_start + (sizeof(char) * a);
                        fseek(archivo, posLectura, SEEK_SET);
                        fread(&c, sizeof(char), 1, archivo);

                        if(c == '\0'){
                            bloque_intermedio = a;
                            c = 'c';
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&c ,sizeof(char), 1 ,archivo);
                            break;
                        }

                        if(a == sblock.s_blocks_count - 1){
                            std::cout << "ERROR: No hay bloques disponibles." << std::endl;
                            fclose(archivo);
                            return;
                        }
                    }

                    for(int a = 0; a < sblock.s_blocks_count; a++){
                        posLectura = sblock.s_bm_block_start + (sizeof(char) * a);
                        fseek(archivo, posLectura, SEEK_SET);
                        fread(&c, sizeof(char), 1, archivo);

                        if(c == '\0'){
                            bloque_usado = a;
                            c = 'c';
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&c ,sizeof(char), 1 ,archivo);
                            break;
                        }

                        if(a == sblock.s_blocks_count - 1){
                            std::cout << "ERROR: No hay bloques disponibles." << std::endl;
                            fclose(archivo);
                            return;
                        }
                    }

                    //Escribir el nuevo bloque de carpeta
                    strcpy(ccarpeta.b_content[0].b_name, path[posicion].c_str());
                    ccarpeta.b_content[0].b_inodo = inodo_temporal;
                    posLectura = sblock.s_block_start + (sizeof(bcarpetas) * bloque_intermedio);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&ccarpeta, sizeof(bcarpetas), 1, archivo);

                    //Escribir el nuevo bloque de apuntadores
                    capuntador_simple.b_pointers[0] = bloque_intermedio;
                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * bloque_apuntadors);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&capuntador_simple, sizeof(bapuntadores), 1, archivo);

                    //Actualizar el inodo
                    linodo.i_block[12] = bloque_apuntadors;
                    linodo.i_mtime = time(NULL);
                    posLectura = sblock.s_inode_start + (sizeof(inodo) * inodo_leido);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&linodo, sizeof(inodo), 1, archivo);

                    //Actualizar el superbloque
                    sblock.s_free_blocks_count -= 3;
                    sblock.s_free_inodes_count -= 1;
                    
                }else{    
                    //Leer el inodo de apuntadores
                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * linodo.i_block[12]);
                    fseek(archivo, posLectura, SEEK_SET);
                    fread(&lapuntador, sizeof(bapuntadores), 1, archivo);

                    //Revisar si hay espacio
                    for(int i = 0; i < 16; i++){ 
                        if(lapuntador.b_pointers[i] == -1){
                            buscar = false;
                            for(int a = 0; a < sblock.s_inodes_count; a++){
                                posLectura = sblock.s_bm_inode_start + (sizeof(char) * a);
                                fseek(archivo, posLectura, SEEK_SET);
                                fread(&c, sizeof(char), 1, archivo);

                                if(c == '\0'){
                                    inodo_temporal = a;
                                    c = '1';
                                    fseek(archivo, posLectura, SEEK_SET);
                                    fwrite(&c ,sizeof(char), 1 ,archivo);
                                    break;
                                }

                                if(a == sblock.s_inodes_count - 1){
                                    std::cout << "ERROR: No hay inodos disponibles." << std::endl;
                                    fclose(archivo);
                                    return;
                                }
                            }

                            for(int a = 0; a < sblock.s_blocks_count; a++){
                                posLectura = sblock.s_bm_block_start + (sizeof(char) * a);
                                fseek(archivo, posLectura, SEEK_SET);
                                fread(&c, sizeof(char), 1, archivo);

                                if(c == '\0'){
                                    bloque_intermedio = a;
                                    c = 'c';
                                    fseek(archivo, posLectura, SEEK_SET);
                                    fwrite(&c ,sizeof(char), 1 ,archivo);
                                    break;
                                }

                                if(a == sblock.s_blocks_count - 1){
                                    std::cout << "ERROR: No hay bloques disponibles." << std::endl;
                                    fclose(archivo);
                                    return;
                                }
                            }

                            for(int a = 0; a < sblock.s_blocks_count; a++){
                                posLectura = sblock.s_bm_block_start + (sizeof(char) * a);
                                fseek(archivo, posLectura, SEEK_SET);
                                fread(&c, sizeof(char), 1, archivo);

                                if(c == '\0'){
                                    bloque_usado = a;
                                    c = 'c';
                                    fseek(archivo, posLectura, SEEK_SET);
                                    fwrite(&c ,sizeof(char), 1 ,archivo);
                                    break;
                                }

                                if(a == sblock.s_blocks_count - 1){
                                    std::cout << "ERROR: No hay bloques disponibles." << std::endl;
                                    fclose(archivo);
                                    return;
                                }
                            }

                            //Escribir el nuevo bloque de carpeta
                            strcpy(ccarpeta.b_content[0].b_name, path[posicion].c_str());
                            ccarpeta.b_content[0].b_inodo = inodo_temporal;
                            posLectura = sblock.s_block_start + (sizeof(bcarpetas) * bloque_intermedio);
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&ccarpeta, sizeof(bcarpetas), 1, archivo);

                            //Actualizar bloque de apuntadores
                            lapuntador.b_pointers[i] = bloque_intermedio;
                            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * linodo.i_block[12]);
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&lapuntador, sizeof(bapuntadores), 1, archivo);

                            //Actualizar el superbloque
                            sblock.s_free_blocks_count -= 2;
                            sblock.s_free_inodes_count -= 1;
                            break;
                        }else{
                            posLectura = sblock.s_block_start + (sizeof(bcarpetas) * lapuntador.b_pointers[i]);
                            fseek(archivo, posLectura, SEEK_SET);
                            fread(&lcarpeta, sizeof(bcarpetas), 1, archivo);

                            for(int j = 0; j < 4; j++){
                                std::string carpeta(lcarpeta.b_content[j].b_name);

                                if(carpeta == "-"){
                                    buscar = false;
                                    
                                    for(int a = 0; a < sblock.s_inodes_count; a++){
                                        posLectura = sblock.s_bm_inode_start + (sizeof(char) * a);
                                        fseek(archivo, posLectura, SEEK_SET);
                                        fread(&c, sizeof(char), 1, archivo);

                                        if(c == '\0'){
                                            inodo_temporal = a;
                                            c = '1';
                                            fseek(archivo, posLectura, SEEK_SET);
                                            fwrite(&c ,sizeof(char), 1 ,archivo);
                                            break;
                                        }

                                        if(a == sblock.s_inodes_count - 1){
                                            std::cout << "ERROR: No hay inodos disponibles." << std::endl;
                                            fclose(archivo);
                                            return;
                                        }
                                    }

                                    for(int a = 0; a < sblock.s_blocks_count; a++){
                                        posLectura = sblock.s_bm_block_start + (sizeof(char) * a);
                                        fseek(archivo, posLectura, SEEK_SET);
                                        fread(&c, sizeof(char), 1, archivo);

                                        if(c == '\0'){
                                            bloque_usado = a;
                                            c = 'c';
                                            fseek(archivo, posLectura, SEEK_SET);
                                            fwrite(&c ,sizeof(char), 1 ,archivo);
                                            break;
                                        }

                                        if(a == sblock.s_blocks_count - 1){
                                            std::cout << "ERROR: No hay bloques disponibles." << std::endl;
                                            fclose(archivo);
                                            return;
                                        }
                                    }

                                    strcpy(lcarpeta.b_content[j].b_name, path[posicion].c_str());
                                    lcarpeta.b_content[j].b_inodo = inodo_temporal;
                                    posLectura = sblock.s_block_start + (sizeof(bcarpetas) * lapuntador.b_pointers[i]);
                                    fseek(archivo, posLectura, SEEK_SET);
                                    fwrite(&lcarpeta, sizeof(bcarpetas), 1, archivo);

                                    //Actualizar el superbloque
                                    sblock.s_free_blocks_count -= 1;
                                    sblock.s_free_inodes_count -= 1;
                                    break;
                                }
                            }

                            if(inodo_temporal != -1){
                                break;
                            }
                        }
                    }
                }
            }

            //Buscar en los indirectos dobles
            if(buscar){
                if(linodo.i_block[13] == -1){
                    buscar = false;
                    for(int a = 0; a < sblock.s_inodes_count; a++){
                        posLectura = sblock.s_bm_inode_start + (sizeof(char) * a);
                        fseek(archivo, posLectura, SEEK_SET);
                        fread(&c, sizeof(char), 1, archivo);

                        if(c == '\0'){
                            inodo_temporal = a;
                            c = '1';
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&c ,sizeof(char), 1 ,archivo);
                            break;
                        }

                        if(a == sblock.s_inodes_count - 1){
                            std::cout << "ERROR: No hay inodos disponibles." << std::endl;
                            fclose(archivo);
                            return;
                        }
                    }

                    for(int a = 0; a < sblock.s_blocks_count; a++){
                        posLectura = sblock.s_bm_block_start + (sizeof(char) * a);
                        fseek(archivo, posLectura, SEEK_SET);
                        fread(&c, sizeof(char), 1, archivo);

                        if(c == '\0'){
                            bloque_apuntadord = a;
                            c = 'p';
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&c ,sizeof(char), 1 ,archivo);
                            break;
                        }

                        if(a == sblock.s_blocks_count - 1){
                            std::cout << "ERROR: No hay bloques disponibles." << std::endl;
                            fclose(archivo);
                            return;
                        }
                    }

                    for(int a = 0; a < sblock.s_blocks_count; a++){
                        posLectura = sblock.s_bm_block_start + (sizeof(char) * a);
                        fseek(archivo, posLectura, SEEK_SET);
                        fread(&c, sizeof(char), 1, archivo);

                        if(c == '\0'){
                            bloque_apuntadors = a;
                            c = 'p';
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&c ,sizeof(char), 1 ,archivo);
                            break;
                        }

                        if(a == sblock.s_blocks_count - 1){
                            std::cout << "ERROR: No hay bloques disponibles." << std::endl;
                            fclose(archivo);
                            return;
                        }
                    }

                    for(int a = 0; a < sblock.s_blocks_count; a++){
                        posLectura = sblock.s_bm_block_start + (sizeof(char) * a);
                        fseek(archivo, posLectura, SEEK_SET);
                        fread(&c, sizeof(char), 1, archivo);

                        if(c == '\0'){
                            bloque_intermedio = a;
                            c = 'c';
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&c ,sizeof(char), 1 ,archivo);
                            break;
                        }

                        if(a == sblock.s_blocks_count - 1){
                            std::cout << "ERROR: No hay bloques disponibles." << std::endl;
                            fclose(archivo);
                            return;
                        }
                    }

                    for(int a = 0; a < sblock.s_blocks_count; a++){
                        posLectura = sblock.s_bm_block_start + (sizeof(char) * a);
                        fseek(archivo, posLectura, SEEK_SET);
                        fread(&c, sizeof(char), 1, archivo);

                        if(c == '\0'){
                            bloque_usado = a;
                            c = 'c';
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&c ,sizeof(char), 1 ,archivo);
                            break;
                        }

                        if(a == sblock.s_blocks_count - 1){
                            std::cout << "ERROR: No hay bloques disponibles." << std::endl;
                            fclose(archivo);
                            return;
                        }
                    }

                    //Escribir el nuevo bloque de carpeta
                    strcpy(ccarpeta.b_content[0].b_name, path[posicion].c_str());
                    ccarpeta.b_content[0].b_inodo = inodo_temporal;
                    posLectura = sblock.s_block_start + (sizeof(bcarpetas) * bloque_intermedio);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&ccarpeta, sizeof(bcarpetas), 1, archivo);

                    //Escribir el nuevo bloque de apuntadores simple
                    capuntador_simple.b_pointers[0] = bloque_intermedio;
                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * bloque_apuntadors);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&capuntador_simple, sizeof(bapuntadores), 1, archivo);

                    //Escribir el nuevo bloque de apuntadores doble
                    capuntador_doble.b_pointers[0] = bloque_apuntadors;
                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * bloque_apuntadord);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&capuntador_doble, sizeof(bapuntadores), 1, archivo);

                    //Actualizar el inodo
                    linodo.i_block[13] = bloque_apuntadord;
                    linodo.i_mtime = time(NULL);
                    posLectura = sblock.s_inode_start + (sizeof(inodo) * inodo_leido);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&linodo, sizeof(inodo), 1, archivo);

                    //Actualizar el superbloque
                    sblock.s_free_blocks_count -= 4;
                    sblock.s_free_inodes_count -= 1;
                    
                }else{    

                    //Leer el inodo de apuntadores dobles
                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * linodo.i_block[13]);
                    fseek(archivo, posLectura, SEEK_SET);
                    fread(&lapuntador_doble, sizeof(bapuntadores), 1, archivo);

                    //Revisar el inodo doble
                    for(int i = 0; i < 16; i++){
                        if(lapuntador_doble.b_pointers[i] == -1){
                            for(int a = 0; a < sblock.s_inodes_count; a++){
                                posLectura = sblock.s_bm_inode_start + (sizeof(char) * a);
                                fseek(archivo, posLectura, SEEK_SET);
                                fread(&c, sizeof(char), 1, archivo);

                                if(c == '\0'){
                                    inodo_temporal = a;
                                    c = '1';
                                    fseek(archivo, posLectura, SEEK_SET);
                                    fwrite(&c ,sizeof(char), 1 ,archivo);
                                    break;
                                }

                                if(a == sblock.s_inodes_count - 1){
                                    std::cout << "ERROR: No hay inodos disponibles." << std::endl;
                                    fclose(archivo);
                                    return;
                                }
                            }

                            for(int a = 0; a < sblock.s_blocks_count; a++){
                                posLectura = sblock.s_bm_block_start + (sizeof(char) * a);
                                fseek(archivo, posLectura, SEEK_SET);
                                fread(&c, sizeof(char), 1, archivo);

                                if(c == '\0'){
                                    bloque_apuntadors = a;
                                    c = 'p';
                                    fseek(archivo, posLectura, SEEK_SET);
                                    fwrite(&c ,sizeof(char), 1 ,archivo);
                                    break;
                                }

                                if(a == sblock.s_blocks_count - 1){
                                    std::cout << "ERROR: No hay bloques disponibles." << std::endl;
                                    fclose(archivo);
                                    return;
                                }
                            }

                            for(int a = 0; a < sblock.s_blocks_count; a++){
                                posLectura = sblock.s_bm_block_start + (sizeof(char) * a);
                                fseek(archivo, posLectura, SEEK_SET);
                                fread(&c, sizeof(char), 1, archivo);

                                if(c == '\0'){
                                    bloque_intermedio = a;
                                    c = 'c';
                                    fseek(archivo, posLectura, SEEK_SET);
                                    fwrite(&c ,sizeof(char), 1 ,archivo);
                                    break;
                                }

                                if(a == sblock.s_blocks_count - 1){
                                    std::cout << "ERROR: No hay bloques disponibles." << std::endl;
                                    fclose(archivo);
                                    return;
                                }
                            }

                            for(int a = 0; a < sblock.s_blocks_count; a++){
                                posLectura = sblock.s_bm_block_start + (sizeof(char) * a);
                                fseek(archivo, posLectura, SEEK_SET);
                                fread(&c, sizeof(char), 1, archivo);

                                if(c == '\0'){
                                    bloque_usado = a;
                                    c = 'c';
                                    fseek(archivo, posLectura, SEEK_SET);
                                    fwrite(&c ,sizeof(char), 1 ,archivo);
                                    break;
                                }

                                if(a == sblock.s_blocks_count - 1){
                                    std::cout << "ERROR: No hay bloques disponibles." << std::endl;
                                    fclose(archivo);
                                    return;
                                }
                            }

                            //Escribir el nuevo bloque de carpeta
                            strcpy(ccarpeta.b_content[0].b_name, path[posicion].c_str());
                            ccarpeta.b_content[0].b_inodo = inodo_temporal;
                            posLectura = sblock.s_block_start + (sizeof(bcarpetas) * bloque_intermedio);
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&ccarpeta, sizeof(bcarpetas), 1, archivo);

                            //Escribir el nuevo bloque de apuntadores simple
                            capuntador_simple.b_pointers[0] = bloque_intermedio;
                            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * bloque_apuntadors);
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&capuntador_simple, sizeof(bapuntadores), 1, archivo);

                            //Actualizar bloque de apuntadores doble
                            lapuntador_doble.b_pointers[i] = bloque_apuntadors;
                            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * linodo.i_block[13]);
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&lapuntador_doble, sizeof(bapuntadores), 1, archivo);

                            //Actualizar el superbloque
                            sblock.s_free_blocks_count -= 3;
                            sblock.s_free_inodes_count -= 1;
                            break;

                        }else{
                            //Leer el inodo de apuntadores 
                            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * lapuntador_doble.b_pointers[i]);
                            fseek(archivo, posLectura, SEEK_SET);
                            fread(&lapuntador, sizeof(bapuntadores), 1, archivo);

                            //Buscar si hay espacio
                            for(int j = 0; j < 16; j++){
                                if(lapuntador.b_pointers[j] == -1){
                                    buscar = false;
                                    for(int a = 0; a < sblock.s_inodes_count; a++){
                                        posLectura = sblock.s_bm_inode_start + (sizeof(char) * a);
                                        fseek(archivo, posLectura, SEEK_SET);
                                        fread(&c, sizeof(char), 1, archivo);

                                        if(c == '\0'){
                                            inodo_temporal = a;
                                            c = '1';
                                            fseek(archivo, posLectura, SEEK_SET);
                                            fwrite(&c ,sizeof(char), 1 ,archivo);
                                            break;
                                        }

                                        if(a == sblock.s_inodes_count - 1){
                                            std::cout << "ERROR: No hay inodos disponibles." << std::endl;
                                            fclose(archivo);
                                            return;
                                        }
                                    }

                                    for(int a = 0; a < sblock.s_blocks_count; a++){
                                        posLectura = sblock.s_bm_block_start + (sizeof(char) * a);
                                        fseek(archivo, posLectura, SEEK_SET);
                                        fread(&c, sizeof(char), 1, archivo);

                                        if(c == '\0'){
                                            bloque_intermedio = a;
                                            c = 'c';
                                            fseek(archivo, posLectura, SEEK_SET);
                                            fwrite(&c ,sizeof(char), 1 ,archivo);
                                            break;
                                        }

                                        if(a == sblock.s_blocks_count - 1){
                                            std::cout << "ERROR: No hay bloques disponibles." << std::endl;
                                            fclose(archivo);
                                            return;
                                        }
                                    }

                                    for(int a = 0; a < sblock.s_blocks_count; a++){
                                        posLectura = sblock.s_bm_block_start + (sizeof(char) * a);
                                        fseek(archivo, posLectura, SEEK_SET);
                                        fread(&c, sizeof(char), 1, archivo);

                                        if(c == '\0'){
                                            bloque_usado = a;
                                            c = 'c';
                                            fseek(archivo, posLectura, SEEK_SET);
                                            fwrite(&c ,sizeof(char), 1 ,archivo);
                                            break;
                                        }

                                        if(a == sblock.s_blocks_count - 1){
                                            std::cout << "ERROR: No hay bloques disponibles." << std::endl;
                                            fclose(archivo);
                                            return;
                                        }
                                    }

                                    //Escribir el nuevo bloque de carpeta
                                    strcpy(ccarpeta.b_content[0].b_name, path[posicion].c_str());
                                    ccarpeta.b_content[0].b_inodo = inodo_temporal;
                                    posLectura = sblock.s_block_start + (sizeof(bcarpetas) * bloque_intermedio);
                                    fseek(archivo, posLectura, SEEK_SET);
                                    fwrite(&ccarpeta, sizeof(bcarpetas), 1, archivo);

                                    //Actualizar bloque de apuntadores
                                    lapuntador.b_pointers[j] = bloque_intermedio;
                                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * lapuntador_doble.b_pointers[i]);
                                    fseek(archivo, posLectura, SEEK_SET);
                                    fwrite(&lapuntador, sizeof(bapuntadores), 1, archivo);

                                    //Actualizar el superbloque
                                    sblock.s_free_blocks_count -= 2;
                                    sblock.s_free_inodes_count -= 1;
                                    break;
                                }else{
                                    posLectura = sblock.s_block_start + (sizeof(bcarpetas) * lapuntador.b_pointers[j]);
                                    fseek(archivo, posLectura, SEEK_SET);
                                    fread(&lcarpeta, sizeof(bcarpetas), 1, archivo);

                                    for(int k = 0; k < 4; k++){
                                        std::string carpeta(lcarpeta.b_content[k].b_name);
                            
                                        if(carpeta == "-"){
                                            buscar = false;

                                            for(int a = 0; a < sblock.s_inodes_count; a++){
                                                posLectura = sblock.s_bm_inode_start + (sizeof(char) * a);
                                                fseek(archivo, posLectura, SEEK_SET);
                                                fread(&c, sizeof(char), 1, archivo);

                                                if(c == '\0'){
                                                    inodo_temporal = a;
                                                    c = '1';
                                                    fseek(archivo, posLectura, SEEK_SET);
                                                    fwrite(&c ,sizeof(char), 1 ,archivo);
                                                    break;
                                                }

                                                if(a == sblock.s_inodes_count - 1){
                                                    std::cout << "ERROR: No hay inodos disponibles." << std::endl;
                                                    fclose(archivo);
                                                    return;
                                                }
                                            }

                                            for(int a = 0; a < sblock.s_blocks_count; a++){
                                                posLectura = sblock.s_bm_block_start + (sizeof(char) * a);
                                                fseek(archivo, posLectura, SEEK_SET);
                                                fread(&c, sizeof(char), 1, archivo);

                                                if(c == '\0'){
                                                    bloque_usado = a;
                                                    c = 'c';
                                                    fseek(archivo, posLectura, SEEK_SET);
                                                    fwrite(&c ,sizeof(char), 1 ,archivo);
                                                    break;
                                                }

                                                if(a == sblock.s_blocks_count - 1){
                                                    std::cout << "ERROR: No hay bloques disponibles." << std::endl;
                                                    fclose(archivo);
                                                    return;
                                                }
                                            }

                                            strcpy(lcarpeta.b_content[k].b_name, path[posicion].c_str());
                                            lcarpeta.b_content[k].b_inodo = inodo_temporal;
                                            posLectura = sblock.s_block_start + (sizeof(bcarpetas) * linodo.i_block[j]);
                                            fseek(archivo, posLectura, SEEK_SET);
                                            fwrite(&lcarpeta, sizeof(bcarpetas), 1, archivo);

                                            //Actualizar el superbloque
                                            sblock.s_free_blocks_count -= 1;
                                            sblock.s_free_inodes_count -= 1;
                                            break;
                                        }
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
                    }
                }
            }

            //Buscar en los indirectos triples 
            if(buscar){
                if(linodo.i_block[14] == -1){
                    buscar = false;
                    for(int a = 0; a < sblock.s_inodes_count; a++){
                        posLectura = sblock.s_bm_inode_start + (sizeof(char) * a);
                        fseek(archivo, posLectura, SEEK_SET);
                        fread(&c, sizeof(char), 1, archivo);

                        if(c == '\0'){
                            inodo_temporal = a;
                            c = '1';
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&c ,sizeof(char), 1 ,archivo);
                            break;
                        }

                        if(a == sblock.s_inodes_count - 1){
                            std::cout << "ERROR: No hay inodos disponibles." << std::endl;
                            fclose(archivo);
                            return;
                        }
                    }

                    for(int a = 0; a < sblock.s_blocks_count; a++){
                        posLectura = sblock.s_bm_block_start + (sizeof(char) * a);
                        fseek(archivo, posLectura, SEEK_SET);
                        fread(&c, sizeof(char), 1, archivo);

                        if(c == '\0'){
                            bloque_apuntadort = a;
                            c = 'p';
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&c ,sizeof(char), 1 ,archivo);
                            break;
                        }

                        if(a == sblock.s_blocks_count - 1){
                            std::cout << "ERROR: No hay bloques disponibles." << std::endl;
                            fclose(archivo);
                            return;
                        }
                    }

                    for(int a = 0; a < sblock.s_blocks_count; a++){
                        posLectura = sblock.s_bm_block_start + (sizeof(char) * a);
                        fseek(archivo, posLectura, SEEK_SET);
                        fread(&c, sizeof(char), 1, archivo);

                        if(c == '\0'){
                            bloque_apuntadord = a;
                            c = 'p';
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&c ,sizeof(char), 1 ,archivo);
                            break;
                        }

                        if(a == sblock.s_blocks_count - 1){
                            std::cout << "ERROR: No hay bloques disponibles." << std::endl;
                            fclose(archivo);
                            return;
                        }
                    }

                    for(int a = 0; a < sblock.s_blocks_count; a++){
                        posLectura = sblock.s_bm_block_start + (sizeof(char) * a);
                        fseek(archivo, posLectura, SEEK_SET);
                        fread(&c, sizeof(char), 1, archivo);

                        if(c == '\0'){
                            bloque_apuntadors = a;
                            c = 'p';
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&c ,sizeof(char), 1 ,archivo);
                            break;
                        }

                        if(a == sblock.s_blocks_count - 1){
                            std::cout << "ERROR: No hay bloques disponibles." << std::endl;
                            fclose(archivo);
                            return;
                        }
                    }

                    for(int a = 0; a < sblock.s_blocks_count; a++){
                        posLectura = sblock.s_bm_block_start + (sizeof(char) * a);
                        fseek(archivo, posLectura, SEEK_SET);
                        fread(&c, sizeof(char), 1, archivo);

                        if(c == '\0'){
                            bloque_intermedio = a;
                            c = 'c';
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&c ,sizeof(char), 1 ,archivo);
                            break;
                        }

                        if(a == sblock.s_blocks_count - 1){
                            std::cout << "ERROR: No hay bloques disponibles." << std::endl;
                            fclose(archivo);
                            return;
                        }
                    }

                    for(int a = 0; a < sblock.s_blocks_count; a++){
                        posLectura = sblock.s_bm_block_start + (sizeof(char) * a);
                        fseek(archivo, posLectura, SEEK_SET);
                        fread(&c, sizeof(char), 1, archivo);

                        if(c == '\0'){
                            bloque_usado = a;
                            c = 'c';
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&c ,sizeof(char), 1 ,archivo);
                            break;
                        }

                        if(a == sblock.s_blocks_count - 1){
                            std::cout << "ERROR: No hay bloques disponibles." << std::endl;
                            fclose(archivo);
                            return;
                        }
                    }

                    //Escribir el nuevo bloque de carpeta
                    strcpy(ccarpeta.b_content[0].b_name, path[posicion].c_str());
                    ccarpeta.b_content[0].b_inodo = inodo_temporal;
                    posLectura = sblock.s_block_start + (sizeof(bcarpetas) * bloque_intermedio);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&ccarpeta, sizeof(bcarpetas), 1, archivo);

                    //Escribir el nuevo bloque de apuntadores simple
                    capuntador_simple.b_pointers[0] = bloque_intermedio;
                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * bloque_apuntadors);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&capuntador_simple, sizeof(bapuntadores), 1, archivo);

                    //Escribir el nuevo bloque de apuntadores doble
                    capuntador_doble.b_pointers[0] = bloque_apuntadors;
                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * bloque_apuntadord);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&capuntador_doble, sizeof(bapuntadores), 1, archivo);

                    //Escribir el nuevo bloque de apuntadores triple
                    capuntador_doble.b_pointers[0] = bloque_apuntadord;
                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * bloque_apuntadort);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&capuntador_triple, sizeof(bapuntadores), 1, archivo);

                    //Actualizar el inodo
                    linodo.i_block[14] = bloque_apuntadord;
                    linodo.i_mtime = time(NULL);
                    posLectura = sblock.s_inode_start + (sizeof(inodo) * inodo_leido);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&linodo, sizeof(inodo), 1, archivo);

                    sblock.s_free_blocks_count -= 5;
                    sblock.s_free_inodes_count -= 1;
                    break;
                    
                }else{

                    //Leer el inodo de apuntadores triple
                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * linodo.i_block[14]);
                    fseek(archivo, posLectura, SEEK_SET);
                    fread(&lapuntador_triple, sizeof(bapuntadores), 1, archivo);

                    //Revisar el inodo triple
                    for(int i = 0; i < 16; i++){
                        if(lapuntador_triple.b_pointers[i] == -1){
                            buscar = false;
                            for(int a = 0; a < sblock.s_inodes_count; a++){
                                posLectura = sblock.s_bm_inode_start + (sizeof(char) * a);
                                fseek(archivo, posLectura, SEEK_SET);
                                fread(&c, sizeof(char), 1, archivo);

                                if(c == '\0'){
                                    inodo_temporal = a;
                                    c = '1';
                                    fseek(archivo, posLectura, SEEK_SET);
                                    fwrite(&c ,sizeof(char), 1 ,archivo);
                                    break;
                                }

                                if(a == sblock.s_inodes_count - 1){
                                    std::cout << "ERROR: No hay inodos disponibles." << std::endl;
                                    fclose(archivo);
                                    return;
                                }
                            }

                            for(int a = 0; a < sblock.s_blocks_count; a++){
                                posLectura = sblock.s_bm_block_start + (sizeof(char) * a);
                                fseek(archivo, posLectura, SEEK_SET);
                                fread(&c, sizeof(char), 1, archivo);

                                if(c == '\0'){
                                    bloque_apuntadord = a;
                                    c = 'p';
                                    fseek(archivo, posLectura, SEEK_SET);
                                    fwrite(&c ,sizeof(char), 1 ,archivo);
                                    break;
                                }

                                if(a == sblock.s_blocks_count - 1){
                                    std::cout << "ERROR: No hay bloques disponibles." << std::endl;
                                    fclose(archivo);
                                    return;
                                }
                            }

                            for(int a = 0; a < sblock.s_blocks_count; a++){
                                posLectura = sblock.s_bm_block_start + (sizeof(char) * a);
                                fseek(archivo, posLectura, SEEK_SET);
                                fread(&c, sizeof(char), 1, archivo);

                                if(c == '\0'){
                                    bloque_apuntadors = a;
                                    c = 'p';
                                    fseek(archivo, posLectura, SEEK_SET);
                                    fwrite(&c ,sizeof(char), 1 ,archivo);
                                    break;
                                }

                                if(a == sblock.s_blocks_count - 1){
                                    std::cout << "ERROR: No hay bloques disponibles." << std::endl;
                                    fclose(archivo);
                                    return;
                                }
                            }

                            for(int a = 0; a < sblock.s_blocks_count; a++){
                                posLectura = sblock.s_bm_block_start + (sizeof(char) * a);
                                fseek(archivo, posLectura, SEEK_SET);
                                fread(&c, sizeof(char), 1, archivo);

                                if(c == '\0'){
                                    bloque_intermedio = a;
                                    c = 'c';
                                    fseek(archivo, posLectura, SEEK_SET);
                                    fwrite(&c ,sizeof(char), 1 ,archivo);
                                    break;
                                }

                                if(a == sblock.s_blocks_count - 1){
                                    std::cout << "ERROR: No hay bloques disponibles." << std::endl;
                                    fclose(archivo);
                                    return;
                                }
                            }

                            for(int a = 0; a < sblock.s_blocks_count; a++){
                                posLectura = sblock.s_bm_block_start + (sizeof(char) * a);
                                fseek(archivo, posLectura, SEEK_SET);
                                fread(&c, sizeof(char), 1, archivo);

                                if(c == '\0'){
                                    bloque_usado = a;
                                    c = 'c';
                                    fseek(archivo, posLectura, SEEK_SET);
                                    fwrite(&c ,sizeof(char), 1 ,archivo);
                                    break;
                                }

                                if(a == sblock.s_blocks_count - 1){
                                    std::cout << "ERROR: No hay bloques disponibles." << std::endl;
                                    fclose(archivo);
                                    return;
                                }
                            }

                            //Escribir el nuevo bloque de carpeta
                            strcpy(ccarpeta.b_content[0].b_name, path[posicion].c_str());
                            ccarpeta.b_content[0].b_inodo = inodo_temporal;
                            posLectura = sblock.s_block_start + (sizeof(bcarpetas) * bloque_intermedio);
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&ccarpeta, sizeof(bcarpetas), 1, archivo);

                            //Escribir el nuevo bloque de apuntadores simple
                            capuntador_simple.b_pointers[0] = bloque_intermedio;
                            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * bloque_apuntadors);
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&capuntador_simple, sizeof(bapuntadores), 1, archivo);

                            //Escribir el nuevo bloque de apuntadores doble
                            capuntador_doble.b_pointers[0] = bloque_apuntadors;
                            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * bloque_apuntadord);
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&capuntador_doble, sizeof(bapuntadores), 1, archivo);

                            //Actualizar bloque de apuntadores triple
                            lapuntador_triple.b_pointers[i] = bloque_apuntadord;
                            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * linodo.i_block[14]);
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&lapuntador_triple, sizeof(bapuntadores), 1, archivo);

                            sblock.s_free_blocks_count -= 4;
                            sblock.s_free_inodes_count -= 1;
                            break;

                        }else{
                            //Leer el bloque de apuntadores 
                            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * lapuntador_triple.b_pointers[i]);
                            fseek(archivo, posLectura, SEEK_SET);
                            fread(&lapuntador_doble, sizeof(bapuntadores), 1, archivo);

                            //Recorrer el bloque de apuntadores dobles 
                            for(int j = 0; j < 16; j++){
                                if(lapuntador_doble.b_pointers[j] == -1){
                                    for(int a = 0; a < sblock.s_inodes_count; a++){
                                        posLectura = sblock.s_bm_inode_start + (sizeof(char) * a);
                                        fseek(archivo, posLectura, SEEK_SET);
                                        fread(&c, sizeof(char), 1, archivo);

                                        if(c == '\0'){
                                            inodo_temporal = a;
                                            c = '1';
                                            fseek(archivo, posLectura, SEEK_SET);
                                            fwrite(&c ,sizeof(char), 1 ,archivo);
                                            break;
                                        }

                                        if(a == sblock.s_inodes_count - 1){
                                            std::cout << "ERROR: No hay inodos disponibles." << std::endl;
                                            fclose(archivo);
                                            return;
                                        }
                                    }

                                    for(int a = 0; a < sblock.s_blocks_count; a++){
                                        posLectura = sblock.s_bm_block_start + (sizeof(char) * a);
                                        fseek(archivo, posLectura, SEEK_SET);
                                        fread(&c, sizeof(char), 1, archivo);

                                        if(c == '\0'){
                                            bloque_apuntadors = a;
                                            c = 'p';
                                            fseek(archivo, posLectura, SEEK_SET);
                                            fwrite(&c ,sizeof(char), 1 ,archivo);
                                            break;
                                        }

                                        if(a == sblock.s_blocks_count - 1){
                                            std::cout << "ERROR: No hay bloques disponibles." << std::endl;
                                            fclose(archivo);
                                            return;
                                        }
                                    }

                                    for(int a = 0; a < sblock.s_blocks_count; a++){
                                        posLectura = sblock.s_bm_block_start + (sizeof(char) * a);
                                        fseek(archivo, posLectura, SEEK_SET);
                                        fread(&c, sizeof(char), 1, archivo);

                                        if(c == '\0'){
                                            bloque_intermedio = a;
                                            c = 'c';
                                            fseek(archivo, posLectura, SEEK_SET);
                                            fwrite(&c ,sizeof(char), 1 ,archivo);
                                            break;
                                        }

                                        if(a == sblock.s_blocks_count - 1){
                                            std::cout << "ERROR: No hay bloques disponibles." << std::endl;
                                            fclose(archivo);
                                            return;
                                        }
                                    }

                                    for(int a = 0; a < sblock.s_blocks_count; a++){
                                        posLectura = sblock.s_bm_block_start + (sizeof(char) * a);
                                        fseek(archivo, posLectura, SEEK_SET);
                                        fread(&c, sizeof(char), 1, archivo);

                                        if(c == '\0'){
                                            bloque_usado = a;
                                            c = 'c';
                                            fseek(archivo, posLectura, SEEK_SET);
                                            fwrite(&c ,sizeof(char), 1 ,archivo);
                                            break;
                                        }

                                        if(a == sblock.s_blocks_count - 1){
                                            std::cout << "ERROR: No hay bloques disponibles." << std::endl;
                                            fclose(archivo);
                                            return;
                                        }
                                    }

                                    //Escribir el nuevo bloque de carpeta
                                    strcpy(ccarpeta.b_content[0].b_name, path[posicion].c_str());
                                    ccarpeta.b_content[0].b_inodo = inodo_temporal;
                                    posLectura = sblock.s_block_start + (sizeof(bcarpetas) * bloque_intermedio);
                                    fseek(archivo, posLectura, SEEK_SET);
                                    fwrite(&ccarpeta, sizeof(bcarpetas), 1, archivo);

                                    //Escribir el nuevo bloque de apuntadores simple
                                    capuntador_simple.b_pointers[0] = bloque_intermedio;
                                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * bloque_apuntadors);
                                    fseek(archivo, posLectura, SEEK_SET);
                                    fwrite(&capuntador_simple, sizeof(bapuntadores), 1, archivo);

                                    //Actualizar bloque de apuntadores doble
                                    lapuntador_doble.b_pointers[j] = bloque_apuntadors;
                                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * lapuntador_triple.b_pointers[i]);
                                    fseek(archivo, posLectura, SEEK_SET);
                                    fwrite(&lapuntador_doble, sizeof(bapuntadores), 1, archivo);

                                    sblock.s_free_blocks_count -= 3;
                                    sblock.s_free_inodes_count -= 1;
                                    break;

                                }else{
                                    //Leer el inodo de apuntadores 
                                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * lapuntador_doble.b_pointers[j]);
                                    fseek(archivo, posLectura, SEEK_SET);
                                    fread(&lapuntador, sizeof(bapuntadores), 1, archivo);

                                    //Buscar si hay espacio
                                    for(int k = 0; k < 16; k++){
                                        if(lapuntador.b_pointers[k] == -1){
                                            buscar = false;
                                            for(int a = 0; a < sblock.s_inodes_count; a++){
                                                posLectura = sblock.s_bm_inode_start + (sizeof(char) * a);
                                                fseek(archivo, posLectura, SEEK_SET);
                                                fread(&c, sizeof(char), 1, archivo);

                                                if(c == '\0'){
                                                    inodo_temporal = a;
                                                    c = '1';
                                                    fseek(archivo, posLectura, SEEK_SET);
                                                    fwrite(&c ,sizeof(char), 1 ,archivo);
                                                    break;
                                                }

                                                if(a == sblock.s_inodes_count - 1){
                                                    std::cout << "ERROR: No hay inodos disponibles." << std::endl;
                                                    fclose(archivo);
                                                    return;
                                                }
                                            }

                                            for(int a = 0; a < sblock.s_blocks_count; a++){
                                                posLectura = sblock.s_bm_block_start + (sizeof(char) * a);
                                                fseek(archivo, posLectura, SEEK_SET);
                                                fread(&c, sizeof(char), 1, archivo);

                                                if(c == '\0'){
                                                    bloque_intermedio = a;
                                                    c = 'c';
                                                    fseek(archivo, posLectura, SEEK_SET);
                                                    fwrite(&c ,sizeof(char), 1 ,archivo);
                                                    break;
                                                }

                                                if(a == sblock.s_blocks_count - 1){
                                                    std::cout << "ERROR: No hay bloques disponibles." << std::endl;
                                                    fclose(archivo);
                                                    return;
                                                }
                                            }

                                            for(int a = 0; a < sblock.s_blocks_count; a++){
                                                posLectura = sblock.s_bm_block_start + (sizeof(char) * a);
                                                fseek(archivo, posLectura, SEEK_SET);
                                                fread(&c, sizeof(char), 1, archivo);

                                                if(c == '\0'){
                                                    bloque_usado = a;
                                                    c = 'c';
                                                    fseek(archivo, posLectura, SEEK_SET);
                                                    fwrite(&c ,sizeof(char), 1 ,archivo);
                                                    break;
                                                }

                                                if(a == sblock.s_blocks_count - 1){
                                                    std::cout << "ERROR: No hay bloques disponibles." << std::endl;
                                                    fclose(archivo);
                                                    return;
                                                }
                                            }

                                            //Escribir el nuevo bloque de carpeta
                                            strcpy(ccarpeta.b_content[0].b_name, path[posicion].c_str());
                                            ccarpeta.b_content[0].b_inodo = inodo_temporal;
                                            posLectura = sblock.s_block_start + (sizeof(bcarpetas) * bloque_intermedio);
                                            fseek(archivo, posLectura, SEEK_SET);
                                            fwrite(&ccarpeta, sizeof(bcarpetas), 1, archivo);

                                            //Actualizar bloque de apuntadores
                                            lapuntador.b_pointers[k] = bloque_intermedio;
                                            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * lapuntador_doble.b_pointers[j]);
                                            fseek(archivo, posLectura, SEEK_SET);
                                            fwrite(&lapuntador, sizeof(bapuntadores), 1, archivo);

                                            sblock.s_free_blocks_count -= 2;
                                            sblock.s_free_inodes_count -= 1;
                                            break;
                                        }else{
                                            posLectura = sblock.s_block_start + (sizeof(bcarpetas) * lapuntador.b_pointers[k]);
                                            fseek(archivo, posLectura, SEEK_SET);
                                            fread(&lcarpeta, sizeof(bcarpetas), 1, archivo);

                                            for(int l = 0; l < 4; l++){
                                                std::string carpeta(lcarpeta.b_content[l].b_name);
                                    
                                                if(carpeta == "-"){
                                                    buscar = false;

                                                    for(int a = 0; a < sblock.s_inodes_count; a++){
                                                        posLectura = sblock.s_bm_inode_start + (sizeof(char) * a);
                                                        fseek(archivo, posLectura, SEEK_SET);
                                                        fread(&c, sizeof(char), 1, archivo);

                                                        if(c == '\0'){
                                                            inodo_temporal = a;
                                                            c = '1';
                                                            fseek(archivo, posLectura, SEEK_SET);
                                                            fwrite(&c ,sizeof(char), 1 ,archivo);
                                                            break;
                                                        }

                                                        if(a == sblock.s_inodes_count - 1){
                                                            std::cout << "ERROR: No hay inodos disponibles." << std::endl;
                                                            fclose(archivo);
                                                            return;
                                                        }
                                                    }

                                                    for(int a = 0; a < sblock.s_blocks_count; a++){
                                                        posLectura = sblock.s_bm_block_start + (sizeof(char) * a);
                                                        fseek(archivo, posLectura, SEEK_SET);
                                                        fread(&c, sizeof(char), 1, archivo);

                                                        if(c == '\0'){
                                                            bloque_usado = a;
                                                            c = 'c';
                                                            fseek(archivo, posLectura, SEEK_SET);
                                                            fwrite(&c ,sizeof(char), 1 ,archivo);
                                                            break;
                                                        }

                                                        if(a == sblock.s_blocks_count - 1){
                                                            std::cout << "ERROR: No hay bloques disponibles." << std::endl;
                                                            fclose(archivo);
                                                            return;
                                                        }
                                                    }

                                                    strcpy(lcarpeta.b_content[l].b_name, path[posicion].c_str());
                                                    lcarpeta.b_content[l].b_inodo = inodo_temporal;
                                                    posLectura = sblock.s_block_start + (sizeof(bcarpetas) * linodo.i_block[k]);
                                                    fseek(archivo, posLectura, SEEK_SET);
                                                    fwrite(&lcarpeta, sizeof(bcarpetas), 1, archivo);
                                                    sblock.s_free_blocks_count -= 1;
                                                    sblock.s_free_inodes_count -= 1;
                                                    break;
                                                }
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
                            }
                        }

                        if(inodo_temporal != -1){
                            break;
                        }
                    }
                }
            }

            //Crear el bloque e inodo de carpeta
            cinodo.i_uid = std::stoi(sesion.id_user);
            cinodo.i_gid = std::stoi(sesion.id_grp);
            cinodo.i_s = 0;
            cinodo.i_atime = time(NULL);
            cinodo.i_ctime = time(NULL);
            cinodo.i_mtime = time(NULL);
            for(int i = 0; i < 15; i++){
                cinodo.i_block[i] = -1;
            }
            cinodo.i_block[0] = bloque_usado;
            cinodo.i_type = '0';
            cinodo.i_perm = 664;
            posLectura = sblock.s_inode_start + (sizeof(inodo) * inodo_temporal);
            fseek(archivo,posLectura , SEEK_SET);
            fwrite(&cinodo, sizeof(inodo), 1, archivo);

            for(int i = 0; i < 4; i++){
                strcpy(ccarpeta.b_content[i].b_name, "-");
                ccarpeta.b_content[i].b_inodo = -1;
            }
            strcpy(ccarpeta.b_content[0].b_name, ".");
            ccarpeta.b_content[0].b_inodo = inodo_temporal;
            strcpy(ccarpeta.b_content[1].b_name, "..");
            ccarpeta.b_content[1].b_inodo = inodo_leido;
            posLectura = sblock.s_block_start + (sizeof(bcarpetas) * bloque_usado);
            fseek(archivo, posLectura, SEEK_SET);
            fwrite(&ccarpeta, sizeof(bcarpetas), 1, archivo);

            //Actualizar Variables
            posLectura = sblock.s_inode_start + (sizeof(inodo) * inodo_temporal);
            fseek(archivo, posLectura, SEEK_SET);
            fread(&linodo, sizeof(inodo), 1, archivo);
            inodo_leido = inodo_temporal;
            posicion += 1;

            //Actualizar el superbloque
            fseek(archivo, posInicio, SEEK_SET);
            fwrite(&sblock, sizeof(sbloque), 1, archivo);
        }
        
        //3. Finalizar el proceso si no existe la carpeta y no esta habilitado -p
        if(inodo_temporal == -1 && !padre){
            continuar = false;
        }

        //4. Determinar si se finaliza el proceso 
        if(linodo.i_type == '1' || posicion == path.size()){
            continuar = false;
        }
    }

    if(error_padre){
        std::cout << "ERROR: Error en la ruta. Una de las carpetas no existe." << std::endl;
        fclose(archivo);
        return;
    }else if(error_permisos){
        std::cout << "ERROR: No posee los permisos para crear la carpeta." << std::endl;
        fclose(archivo);
        return;
    }else if(posicion != path.size()){
        std::cout << "ERROR: Ocurrió un error. No se pudo crear la carpeta." << std::endl;
        fclose(archivo);
        return;
    }
    
    //ESCRIBIR EN EL JOURNAL EL COMANDO Nota: P = con atributo -p NP = sin atributo -p
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
            std::string contenido;
            if(padre){
                contenido = "P";
            }else{
                contenido = "NP";
            }
            strcpy(creacion.comando ,"mkdir");
            strcpy(creacion.path ,ruta.c_str());
            strcpy(creacion.nombre ,"");
            strcpy(creacion.contenido, contenido.c_str());
            creacion.fecha = time(0);
            fseek(archivo, posRegistro, SEEK_SET);
            fwrite(&creacion, sizeof(registro), 1, archivo);
        }
    }

    std::cout << "MENSAJE: Carpeta(s) creada(s) correctamente." << std::endl;
    fclose(archivo);

}