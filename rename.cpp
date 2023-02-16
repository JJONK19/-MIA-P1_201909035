#include "rename.h"

void rename(std::vector<std::string> &parametros, std::vector<disco> &discos, usuario &sesion){
    
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
    std::string nombre = "";                   //Atributo name
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
    int inodo_padre = 0;                       //Inodo de la carpeta que contiene el archivo/carpeta a borrar                      

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
        }else if(tag == "name"){
            nombre = value;
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
        std::cout << "ERROR: La instrucción rename carece de todos los parametros obligatorios." << std::endl;
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

    //BUSCAR LA CARPETA A RENOMBRAR
    posicion = 1;
    continuar = true;
    if(ruta == "/"){
        std::cout << "ERROR: No se puede renombrar la raíz." << std::endl;
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
    

    //REVISAR PERMISOS EN EL PADRE
    posLectura = sblock.s_inode_start + (sizeof(inodo) * inodo_leido);
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
        std::cout << "ERROR: No posee permisos para renombrar." << std::endl;
        fclose(archivo);
        return;
    }

    //Leer el inodo del padre
    posLectura = sblock.s_inode_start + (sizeof(inodo) * inodo_padre);
    fseek(archivo, posLectura, SEEK_SET);
    fread(&linodo, sizeof(inodo), 1, archivo);

    //REVISAR SI EL NOMBRE NO ESTÁ REPETIDO
    int contador = 0;
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

                    if(carpeta == nombre){
                        contador += 1;
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

                        if(carpeta == nombre){
                            contador += 1;
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

                            if(carpeta == nombre){
                                contador += 1;
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

                if(carpeta == nombre){
                    contador += 1;
                }
            }
        }
    }

    if(contador != 0){
        std::cout << "ERROR: El nombre ya se encuentra en uso." << std::endl;
        fclose(archivo);
        return;
    }

    //RENONMBRAR LA CARPETA
    //Contenido: El nombre del nuevo archivo
    //Buscar el archivo y renombrarlo
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

                        //Renombrar de la carpeta
                        strcpy(lcarpeta.b_content[k].b_name, nombre.c_str());
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

                            //Renombrar de la carpeta
                            strcpy(lcarpeta.b_content[l].b_name, nombre.c_str());
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

                                //Renombrar de la carpeta
                                strcpy(lcarpeta.b_content[m].b_name, nombre.c_str());
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

                    //Renombrar de la carpeta
                    strcpy(lcarpeta.b_content[j].b_name, nombre.c_str());
                    posLectura = sblock.s_block_start + (sizeof(bcarpetas) * linodo.i_block[i]);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&lcarpeta, sizeof(bcarpetas), 1, archivo);
                    break;
                }
            }
        }
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
            std::string contenido = sesion.id_user;
            contenido.append(",");
            contenido.append(sesion.user);
            contenido.append(",");
            contenido.append(sesion.id_grp);
            contenido.append(",");
            contenido.append(sesion.grupo);
            contenido.append(",");
            contenido.append(sesion.disco);
            contenido.append(",");
            contenido.append(nombre);

            strcpy(creacion.comando ,"rename");
            strcpy(creacion.path ,ruta.c_str());
            strcpy(creacion.nombre ,nombre_archivo.c_str());
            strcpy(creacion.contenido, contenido.c_str());
            creacion.fecha = time(NULL);
            fseek(archivo, posRegistro, SEEK_SET);
            fwrite(&creacion, sizeof(registro), 1, archivo);
        }
    }

    fclose(archivo);
    std::cout << "MENSAJE: Archivo/Carpeta renombrado correctamente." << std::endl;
}
