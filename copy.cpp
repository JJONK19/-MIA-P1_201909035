#include "copy.h"

void copy(std::vector<std::string> &parametros, std::vector<disco> &discos, usuario &sesion){
    
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
    std::string destino = "";                  //Atributo destino
    std::string diskName = "";                 //Nombre del disco
    int posDisco = -1;                         //Posicion del disco dentro del vector
    int posParticion = -1;                     //Posicion de la particion dentro del vector del disco
    int posInicio;                             //Posicion donde inicia la particion
    int posLectura;                            //Para determinar la posicion de lectura en disco
    sbloque sblock;                            //Para leer el superbloque
    inodo linodo;                              //Para el manejo de los inodos
    inodo tinodo;                              //Inodo para leer el inodo a copiar
    bcarpetas lcarpeta;                        //Para el manejo de bloques de carpetas
    barchivos larchivo;                        //Para el manejo de bloques de archivo
    bapuntadores lapuntador;                   //Para el manejo de bloques de apuntadores simples
    bapuntadores lapuntador_doble;             //Para el manejo de bloques de apuntadores dobles
    bapuntadores lapuntador_triple;            //Para el manejo de bloques de apuntadores triples
    bool continuar = true;                     //Usado como bandera al buscar la ruta
    std::string nombre_archivo;                //Nombre del archivo sin la ruta
    int inodo_copiar = -1;                     //Inodo que se va a copiar
    int inodo_destino = -1;                    //Inodo donde se van a añadir los archivos
    
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
        }else if(tag == "destino"){
            destino = value;
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
    if(ruta == "" || destino == ""){
        required = false;
    }

    if(!required){
        std::cout << "ERROR: La instrucción copy carece de todos los parametros obligatorios." << std::endl;
    }

    //PREPARACIÓN DE PARAMETROS - Extraer de la ruta el nombre del archivo 
    nombre_archivo = ruta.substr(ruta.find_last_of("\\/") + 1, ruta.size() - 1);
    
    //PREPARACIÓN DE PARAMETROS - Separar los nombres que vengan en las rutas.
    std::regex separador("/");
    std::vector<std::string> path(std::sregex_token_iterator(ruta.begin(), ruta.end(), separador, -1),
                    std::sregex_token_iterator());

    std::vector<std::string> path_destino(std::sregex_token_iterator(destino.begin(), destino.end(), separador, -1),
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

    //BUSCAR LA CARPETA A COPIAR
    posicion = 1;
    continuar = true;
    if(ruta == "/"){
        std::cout << "ERROR: No se puede copiar la raíz." << std::endl;
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
            if(inodo_copiar != -1 || inodo_temporal != -1){
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
                                inodo_copiar = lcarpeta.b_content[k].b_inodo;
                                posicion += 1;
                                continuar = false;
                                break;
                            }else{
                                inodo_temporal = lcarpeta.b_content[k].b_inodo;
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
                                    inodo_copiar = lcarpeta.b_content[l].b_inodo;
                                    posicion += 1;
                                    continuar = false;
                                    break;
                                }else{
                                    inodo_temporal = lcarpeta.b_content[l].b_inodo;
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
                                        inodo_copiar = lcarpeta.b_content[m].b_inodo;
                                        posicion += 1;
                                        continuar = false;
                                        break;
                                    }else{
                                        inodo_temporal = lcarpeta.b_content[m].b_inodo;
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
                            inodo_copiar = lcarpeta.b_content[j].b_inodo;
                            posicion += 1;
                            continuar = false;
                            break;
                        }else{
                            inodo_temporal = lcarpeta.b_content[j].b_inodo;
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
            std::cout << "ERROR: La ruta ingresada del contenido no existe." << std::endl;
            fclose(archivo);
            return;
        }else if(posicion == path.size() && inodo_copiar == -1){
            continuar = false;
            std::cout << "ERROR: La ruta del contenido es erronea." << std::endl;
            fclose(archivo);
            return;
        }
    }
    
    //LEER INODO DEL CONTENIDO
    posLectura = sblock.s_inode_start + (sizeof(inodo) * inodo_copiar);
    fseek(archivo, posLectura, SEEK_SET);
    fread(&tinodo, sizeof(inodo), 1, archivo);
    
    //LEER EL INODO RAIZ
    posLectura = sblock.s_inode_start;
    fseek(archivo, posLectura, SEEK_SET);
    fread(&linodo, sizeof(inodo), 1, archivo);

    //BUSCAR EL DESTINO 
    posicion = 1;
    continuar = true;
    if(destino == "/"){
        inodo_destino = 0;
        continuar = false;
    }else if(path_destino[0] != "\0"){
        std::cout << "ERROR: La ruta ingresada del contenedor es erronea." << std::endl;
        fclose(archivo);
        return;
    }

    while(continuar){
        int inodo_temporal = -1;
        
        for(int i = 0; i < 15; i++){
            if(inodo_destino != -1 || inodo_temporal != -1){
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

                        if(carpeta == path_destino[posicion]){
                            if(posicion == path_destino.size() - 1){
                                inodo_destino = lcarpeta.b_content[k].b_inodo;
                                posicion += 1;
                                continuar = false;
                                break;
                            }else{
                                inodo_temporal = lcarpeta.b_content[k].b_inodo;
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

                            if(carpeta == path_destino[posicion]){
                                if(posicion == path_destino.size() - 1){
                                    inodo_destino = lcarpeta.b_content[l].b_inodo;
                                    posicion += 1;
                                    continuar = false;
                                    break;
                                }else{
                                    inodo_temporal = lcarpeta.b_content[l].b_inodo;
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

                                if(carpeta == path_destino[posicion]){
                                    if(posicion == path_destino.size() - 1){
                                        inodo_destino = lcarpeta.b_content[m].b_inodo;
                                        posicion += 1;
                                        continuar = false;
                                        break;
                                    }else{
                                        inodo_temporal = lcarpeta.b_content[m].b_inodo;
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
        
                    if(carpeta == path_destino[posicion]){
                        if(posicion == path_destino.size() - 1){
                            inodo_destino = lcarpeta.b_content[j].b_inodo;
                            posicion += 1;
                            continuar = false;
                            break;
                        }else{
                            inodo_temporal = lcarpeta.b_content[j].b_inodo;
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

        if(inodo_temporal == -1 && posicion != path_destino.size()){
            continuar = false;
            std::cout << "ERROR: La ruta ingresada del contenedor no existe." << std::endl;
            fclose(archivo);
            return;
        }else if(posicion == path_destino.size() && inodo_destino == -1){
            continuar = false;
            std::cout << "ERROR: La ruta del contenedor es erronea." << std::endl;
            fclose(archivo);
            return;
        }
    }
    
    //LEER INODO DEL CONTENEDOR
    posLectura = sblock.s_inode_start + (sizeof(inodo) * inodo_destino);
    fseek(archivo, posLectura, SEEK_SET);
    fread(&linodo, sizeof(inodo), 1, archivo);

    //VERIFICAR QUE SEA UNA CARPETA
    if(linodo.i_type == '1'){
        std::cout << "ERROR: El destino debe de ser una carpeta." << std::endl;
        fclose(archivo);
        return; 
    }

    //VERIFICAR QUE POSEA PERMISO PARA ESCRIBIR 
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
        std::cout << "ERROR: No posee permisos para escribir en la carpeta destino." << std::endl;
        fclose(archivo);
        return;
    }
    
    //BUSCAR UN ESPACIO EN LA CARPETA Y AÑADIR EL ARCHIVO
    int inodo_temporal = -1;
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
        int nuevo_inodo;

        if(linodo.i_block[i] != -1){
            posLectura = sblock.s_block_start + (sizeof(bcarpetas) * linodo.i_block[i]);
            fseek(archivo, posLectura, SEEK_SET);
            fread(&lcarpeta, sizeof(bcarpetas), 1, archivo);

            for(int j = 0; j < 4; j++){
                std::string carpeta(lcarpeta.b_content[j].b_name);
                
                if(carpeta == "-"){
                    buscar = false;
                    

                    if(tinodo.i_type == '1'){
                        nuevo_inodo = archivos(sesion, sblock, tempD.ruta, inodo_copiar); 
                    }else{
                        nuevo_inodo = carpetas(sesion, sblock, tempD.ruta, inodo_copiar, inodo_destino);
                    }

                    if(nuevo_inodo == -1){
                        fclose(archivo);
                        return;
                    }

                    strcpy(lcarpeta.b_content[j].b_name, nombre_archivo.c_str());
                    lcarpeta.b_content[j].b_inodo = nuevo_inodo;
                    posLectura = sblock.s_block_start + (sizeof(bcarpetas) * linodo.i_block[i]);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&lcarpeta, sizeof(bcarpetas), 1, archivo);
                    break;
                }
            }
        }else{
            buscar = false;
            

            if(tinodo.i_type == '1'){
                nuevo_inodo = archivos(sesion, sblock, tempD.ruta, inodo_copiar); 
            }else{
                nuevo_inodo = carpetas(sesion, sblock, tempD.ruta, inodo_copiar, inodo_destino);
            }

            if(nuevo_inodo == -1){
                fclose(archivo);
                return;
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

            //Escribir el nuevo bloque de carpeta
            strcpy(ccarpeta.b_content[0].b_name, nombre_archivo.c_str());
            ccarpeta.b_content[0].b_inodo = nuevo_inodo;
            posLectura = sblock.s_block_start + (sizeof(bcarpetas) * bloque_intermedio);
            fseek(archivo, posLectura, SEEK_SET);
            fwrite(&ccarpeta, sizeof(bcarpetas), 1, archivo);

            //Actualizar el inodo
            linodo.i_block[i] = bloque_intermedio;
            linodo.i_mtime = time(NULL);
            posLectura = sblock.s_inode_start + (sizeof(inodo) * inodo_destino);
            fseek(archivo, posLectura, SEEK_SET);
            fwrite(&linodo, sizeof(inodo), 1, archivo);

            //Actualizar el superbloque
            sblock.s_free_blocks_count -= 1;
            break;
        }

        if(!buscar){
            break;
        }
    }   

    //Buscar en los indirectos simples
    if(buscar){
        int nuevo_inodo;
        if(linodo.i_block[12] == -1){
            buscar = false;
            

            if(tinodo.i_type == '1'){
                nuevo_inodo = archivos(sesion, sblock, tempD.ruta, inodo_copiar); 
            }else{
                nuevo_inodo = carpetas(sesion, sblock, tempD.ruta, inodo_copiar, inodo_destino);
            }

            if(nuevo_inodo == -1){
                fclose(archivo);
                return;
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

            //Escribir el nuevo bloque de carpeta
            strcpy(ccarpeta.b_content[0].b_name, nombre_archivo.c_str());
            ccarpeta.b_content[0].b_inodo = nuevo_inodo;
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
            posLectura = sblock.s_inode_start + (sizeof(inodo) * inodo_destino);
            fseek(archivo, posLectura, SEEK_SET);
            fwrite(&linodo, sizeof(inodo), 1, archivo);

            //Actualizar el superbloque
            sblock.s_free_blocks_count -= 2;
            
        }else{    
            //Leer el inodo de apuntadores
            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * linodo.i_block[12]);
            fseek(archivo, posLectura, SEEK_SET);
            fread(&lapuntador, sizeof(bapuntadores), 1, archivo);

            //Revisar si hay espacio
            for(int i = 0; i < 16; i++){ 
                if(lapuntador.b_pointers[i] == -1){
                    buscar = false;
                    

                    if(tinodo.i_type == '1'){
                        nuevo_inodo = archivos(sesion, sblock, tempD.ruta, inodo_copiar); 
                    }else{
                        nuevo_inodo = carpetas(sesion, sblock, tempD.ruta, inodo_copiar, inodo_destino);
                    }

                    if(nuevo_inodo == -1){
                        fclose(archivo);
                        return;
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

                    //Escribir el nuevo bloque de carpeta
                    strcpy(ccarpeta.b_content[0].b_name, nombre_archivo.c_str());
                    ccarpeta.b_content[0].b_inodo = nuevo_inodo;
                    posLectura = sblock.s_block_start + (sizeof(bcarpetas) * bloque_intermedio);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&ccarpeta, sizeof(bcarpetas), 1, archivo);

                    //Actualizar bloque de apuntadores
                    lapuntador.b_pointers[i] = bloque_intermedio;
                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * linodo.i_block[12]);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&lapuntador, sizeof(bapuntadores), 1, archivo);

                    //Actualizar el superbloque
                    sblock.s_free_blocks_count -= 1;
                    break;
                }else{
                    posLectura = sblock.s_block_start + (sizeof(bcarpetas) * lapuntador.b_pointers[i]);
                    fseek(archivo, posLectura, SEEK_SET);
                    fread(&lcarpeta, sizeof(bcarpetas), 1, archivo);

                    for(int j = 0; j < 4; j++){
                        std::string carpeta(lcarpeta.b_content[j].b_name);

                        if(carpeta == "-"){
                            buscar = false;
                            

                            if(tinodo.i_type == '1'){
                                nuevo_inodo = archivos(sesion, sblock, tempD.ruta, inodo_copiar); 
                            }else{
                                nuevo_inodo = carpetas(sesion, sblock, tempD.ruta, inodo_copiar, inodo_destino);
                            }

                            if(nuevo_inodo == -1){
                                fclose(archivo);
                                return;
                            }

                            strcpy(lcarpeta.b_content[j].b_name, nombre_archivo.c_str());
                            lcarpeta.b_content[j].b_inodo = nuevo_inodo;
                            posLectura = sblock.s_block_start + (sizeof(bcarpetas) * lapuntador.b_pointers[i]);
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&lcarpeta, sizeof(bcarpetas), 1, archivo);

                            //Actualizar el superbloque
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
        int nuevo_inodo;
        if(linodo.i_block[13] == -1){
            buscar = false;
            

            if(tinodo.i_type == '1'){
                nuevo_inodo = archivos(sesion, sblock, tempD.ruta, inodo_copiar); 
            }else{
                nuevo_inodo = carpetas(sesion, sblock, tempD.ruta, inodo_copiar, inodo_destino);
            }

            if(nuevo_inodo == -1){
                fclose(archivo);
                return;
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

            //Escribir el nuevo bloque de carpeta
            strcpy(ccarpeta.b_content[0].b_name, nombre_archivo.c_str());
            ccarpeta.b_content[0].b_inodo = nuevo_inodo;
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
            posLectura = sblock.s_inode_start + (sizeof(inodo) * inodo_destino);
            fseek(archivo, posLectura, SEEK_SET);
            fwrite(&linodo, sizeof(inodo), 1, archivo);

            //Actualizar el superbloque
            sblock.s_free_blocks_count -= 3;
            
        }else{    

            //Leer el inodo de apuntadores dobles
            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * linodo.i_block[13]);
            fseek(archivo, posLectura, SEEK_SET);
            fread(&lapuntador_doble, sizeof(bapuntadores), 1, archivo);

            //Revisar el inodo doble
            for(int i = 0; i < 16; i++){
                if(lapuntador_doble.b_pointers[i] == -1){
                    buscar = false;
                    

                    if(tinodo.i_type == '1'){
                        nuevo_inodo = archivos(sesion, sblock, tempD.ruta, inodo_copiar); 
                    }else{
                        nuevo_inodo = carpetas(sesion, sblock, tempD.ruta, inodo_copiar, inodo_destino);
                    }

                    if(nuevo_inodo == -1){
                        fclose(archivo);
                        return;
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

                    //Escribir el nuevo bloque de carpeta
                    strcpy(ccarpeta.b_content[0].b_name, nombre_archivo.c_str());
                    ccarpeta.b_content[0].b_inodo = nuevo_inodo;
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
                    sblock.s_free_blocks_count -= 2;
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
                            

                            if(tinodo.i_type == '1'){
                                nuevo_inodo = archivos(sesion, sblock, tempD.ruta, inodo_copiar); 
                            }else{
                                nuevo_inodo = carpetas(sesion, sblock, tempD.ruta, inodo_copiar, inodo_destino);
                            }

                            if(nuevo_inodo == -1){
                                fclose(archivo);
                                return;
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

                            //Escribir el nuevo bloque de carpeta
                            strcpy(ccarpeta.b_content[0].b_name, nombre_archivo.c_str());
                            ccarpeta.b_content[0].b_inodo = nuevo_inodo;
                            posLectura = sblock.s_block_start + (sizeof(bcarpetas) * bloque_intermedio);
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&ccarpeta, sizeof(bcarpetas), 1, archivo);

                            //Actualizar bloque de apuntadores
                            lapuntador.b_pointers[j] = bloque_intermedio;
                            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * lapuntador_doble.b_pointers[i]);
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&lapuntador, sizeof(bapuntadores), 1, archivo);

                            //Actualizar el superbloque
                            sblock.s_free_blocks_count -= 1;
                            break;
                        }else{
                            posLectura = sblock.s_block_start + (sizeof(bcarpetas) * lapuntador.b_pointers[j]);
                            fseek(archivo, posLectura, SEEK_SET);
                            fread(&lcarpeta, sizeof(bcarpetas), 1, archivo);

                            for(int k = 0; k < 4; k++){
                                std::string carpeta(lcarpeta.b_content[k].b_name);
                    
                                if(carpeta == "-"){
                                    buscar = false;
                                    

                                    if(tinodo.i_type == '1'){
                                        nuevo_inodo = archivos(sesion, sblock, tempD.ruta, inodo_copiar); 
                                    }else{
                                        nuevo_inodo = carpetas(sesion, sblock, tempD.ruta, inodo_copiar, inodo_destino);
                                    }

                                    if(nuevo_inodo == -1){
                                        fclose(archivo);
                                        return;
                                    }

                                    strcpy(lcarpeta.b_content[k].b_name, nombre_archivo.c_str());
                                    lcarpeta.b_content[k].b_inodo = nuevo_inodo;
                                    posLectura = sblock.s_block_start + (sizeof(bcarpetas) * linodo.i_block[j]);
                                    fseek(archivo, posLectura, SEEK_SET);
                                    fwrite(&lcarpeta, sizeof(bcarpetas), 1, archivo);
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
        int nuevo_inodo;
        if(linodo.i_block[14] == -1){
            buscar = false;
            

            if(tinodo.i_type == '1'){
                nuevo_inodo = archivos(sesion, sblock, tempD.ruta, inodo_copiar); 
            }else{
                nuevo_inodo = carpetas(sesion, sblock, tempD.ruta, inodo_copiar, inodo_destino);
            }

            if(nuevo_inodo == -1){
                fclose(archivo);
                return;
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

            //Escribir el nuevo bloque de carpeta
            strcpy(ccarpeta.b_content[0].b_name, nombre_archivo.c_str());
            ccarpeta.b_content[0].b_inodo = nuevo_inodo;
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
            posLectura = sblock.s_inode_start + (sizeof(inodo) * inodo_destino);
            fseek(archivo, posLectura, SEEK_SET);
            fwrite(&linodo, sizeof(inodo), 1, archivo);

            sblock.s_free_blocks_count -= 5;
            
        }else{

            //Leer el inodo de apuntadores triple
            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * linodo.i_block[14]);
            fseek(archivo, posLectura, SEEK_SET);
            fread(&lapuntador_triple, sizeof(bapuntadores), 1, archivo);

            //Revisar el inodo triple
            for(int i = 0; i < 16; i++){
                if(lapuntador_triple.b_pointers[i] == -1){
                    buscar = false;
                    

                    if(tinodo.i_type == '1'){
                        nuevo_inodo = archivos(sesion, sblock, tempD.ruta, inodo_copiar); 
                    }else{
                        nuevo_inodo = carpetas(sesion, sblock, tempD.ruta, inodo_copiar, inodo_destino);
                    }

                    if(nuevo_inodo == -1){
                        fclose(archivo);
                        return;
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

                    //Escribir el nuevo bloque de carpeta
                    strcpy(ccarpeta.b_content[0].b_name, nombre_archivo.c_str());
                    ccarpeta.b_content[0].b_inodo = nuevo_inodo;
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

                    sblock.s_free_blocks_count -= 3;
                    break;

                }else{
                    //Leer el bloque de apuntadores 
                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * lapuntador_triple.b_pointers[i]);
                    fseek(archivo, posLectura, SEEK_SET);
                    fread(&lapuntador_doble, sizeof(bapuntadores), 1, archivo);

                    //Recorrer el bloque de apuntadores dobles 
                    for(int j = 0; j < 16; j++){
                        if(lapuntador_doble.b_pointers[j] == -1){
                            buscar = false;
                            

                            if(tinodo.i_type == '1'){
                                nuevo_inodo = archivos(sesion, sblock, tempD.ruta, inodo_copiar); 
                            }else{
                                nuevo_inodo = carpetas(sesion, sblock, tempD.ruta, inodo_copiar, inodo_destino);
                            }

                            if(nuevo_inodo == -1){
                                fclose(archivo);
                                return;
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

                            //Escribir el nuevo bloque de carpeta
                            strcpy(ccarpeta.b_content[0].b_name, nombre_archivo.c_str());
                            ccarpeta.b_content[0].b_inodo = nuevo_inodo;
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

                            sblock.s_free_blocks_count -= 2;
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
                                    

                                    if(tinodo.i_type == '1'){
                                        nuevo_inodo = archivos(sesion, sblock, tempD.ruta, inodo_copiar); 
                                    }else{
                                        nuevo_inodo = carpetas(sesion, sblock, tempD.ruta, inodo_copiar, inodo_destino);
                                    }

                                    if(nuevo_inodo == -1){
                                        fclose(archivo);
                                        return;
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

                                    //Escribir el nuevo bloque de carpeta
                                    strcpy(ccarpeta.b_content[0].b_name, nombre_archivo.c_str());
                                    ccarpeta.b_content[0].b_inodo = nuevo_inodo;
                                    posLectura = sblock.s_block_start + (sizeof(bcarpetas) * bloque_intermedio);
                                    fseek(archivo, posLectura, SEEK_SET);
                                    fwrite(&ccarpeta, sizeof(bcarpetas), 1, archivo);

                                    //Actualizar bloque de apuntadores
                                    lapuntador.b_pointers[k] = bloque_intermedio;
                                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * lapuntador_doble.b_pointers[j]);
                                    fseek(archivo, posLectura, SEEK_SET);
                                    fwrite(&lapuntador, sizeof(bapuntadores), 1, archivo);

                                    sblock.s_free_blocks_count -= 1;
                                    break;
                                }else{
                                    posLectura = sblock.s_block_start + (sizeof(bcarpetas) * lapuntador.b_pointers[k]);
                                    fseek(archivo, posLectura, SEEK_SET);
                                    fread(&lcarpeta, sizeof(bcarpetas), 1, archivo);

                                    for(int l = 0; l < 4; l++){
                                        std::string carpeta(lcarpeta.b_content[l].b_name);
                            
                                        if(carpeta == "-"){
                                            buscar = false;
                                            

                                            if(tinodo.i_type == '1'){
                                                nuevo_inodo = archivos(sesion, sblock, tempD.ruta, inodo_copiar); 
                                            }else{
                                                nuevo_inodo = carpetas(sesion, sblock, tempD.ruta, inodo_copiar, inodo_destino);
                                            }

                                            if(nuevo_inodo == -1){
                                                fclose(archivo);
                                                return;
                                            }

                                            strcpy(lcarpeta.b_content[l].b_name, nombre_archivo.c_str());
                                            lcarpeta.b_content[l].b_inodo = nuevo_inodo;
                                            posLectura = sblock.s_block_start + (sizeof(bcarpetas) * linodo.i_block[k]);
                                            fseek(archivo, posLectura, SEEK_SET);
                                            fwrite(&lcarpeta, sizeof(bcarpetas), 1, archivo);
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

    //ACTUALIZAR EL SUPERBLOQUE
    fseek(archivo, posInicio, SEEK_SET);
    fwrite(&sblock, sizeof(sbloque), 1, archivo);
 

    //ESCRIBIR EN EL JOURNAL EL COMANDO
    //Contenido: destino
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
            contenido.append(destino);

            strcpy(creacion.comando ,"copy");
            strcpy(creacion.path ,ruta.c_str());
            strcpy(creacion.nombre ,nombre_archivo.c_str());
            strcpy(creacion.contenido, contenido.c_str());
            creacion.fecha = time(NULL);
            fseek(archivo, posRegistro, SEEK_SET);
            fwrite(&creacion, sizeof(registro), 1, archivo);
        }
    }

    fclose(archivo);
    std::cout << "MENSAJE: Archivo/Carpeta copiado correctamente." << std::endl;
}

int carpetas(usuario &sesion, sbloque &sblock, std::string &ruta, int &no_inodo, int &padre_inodo){
    //VARIABLES
    FILE *archivo;                             //Sirve para verificar que el archivo exista
    int posLectura;                            //Para determinar la posicion de lectura en disco
    inodo linodo;                              //Para el manejo de los inodos
    inodo cinodo;                              //Inodo nuevo que se va a copiar
    inodo tinodo;                              //Inodo nuevo que se va a copiar
    bcarpetas lcarpeta;                        //Para el manejo de bloques de archivo
    bapuntadores lapuntador;                   //Para el manejo de bloques de apuntadores simples
    bapuntadores lapuntador_doble;             //Para el manejo de bloques de apuntadores dobles
    bapuntadores lapuntador_triple;            //Para el manejo de bloques de apuntadores triples
    bool continuar = true;                     //Usado como bandera al buscar la ruta
    int ninodo = -1;                            //Número de Inodo que se va a usar
    char c;

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
        if(permisos[0] == '4' || permisos[0] == '5' || permisos[0] == '6' || permisos[0] == '7'){
            acceso = true;
        }
    }else if(ugo == 2){
        if(permisos[1] == '4' || permisos[1] == '5' || permisos[1] == '6' || permisos[1] == '7'){
            acceso = true;
        }
    }else{
        if(permisos[2] == '4' || permisos[2] == '5' || permisos[2] == '6' || permisos[2] == '7'){
            acceso = true;
        }
    }

    if(sesion.user == "root"){
        acceso = true;
    }

    if(!acceso){
        std::cout << "ERROR: No posee permisos para copiar el archivo." << std::endl;
        fclose(archivo);
        return ninodo;
    }

    //COPIAR EL INODO
    //Buscar un espacio
    for(int a = 0; a < sblock.s_inodes_count; a++){
        posLectura = sblock.s_bm_inode_start + (sizeof(char) * a);
        fseek(archivo, posLectura, SEEK_SET);
        fread(&c, sizeof(char), 1, archivo);

        if(c == '\0'){
            ninodo = a;
            c = '1';
            fseek(archivo, posLectura, SEEK_SET);
            fwrite(&c ,sizeof(char), 1 ,archivo);
            break;
        }

        if(a == sblock.s_inodes_count - 1){
            std::cout << "ERROR: No hay inodos disponibles." << std::endl;
            fclose(archivo);
            return ninodo;
        }
    }
    
    //Escribir sus datos
    cinodo.i_uid = linodo.i_uid;
    cinodo.i_gid = linodo.i_gid;
    cinodo.i_s = linodo.i_s;
    cinodo.i_atime = linodo.i_atime;
    cinodo.i_ctime = linodo.i_ctime;
    cinodo.i_mtime = linodo.i_mtime;
    for(int i = 0; i < 15; i++){
        cinodo.i_block[i] = -1;
    }
    cinodo.i_type = linodo.i_type;
    cinodo.i_perm = linodo.i_perm;
    
    //Copiar el contenido
    for(int i = 0; i < 15; i++){
        int bloque_usado = -1;
        int bloque_apuntadors = -1;
        int bloque_apuntadord = -1;
        int bloque_apuntadort = -1;
        bcarpetas ecarpeta;
        bapuntadores eapuntador;
        bapuntadores eapuntador_doble;
        bapuntadores eapuntador_triple;

        if(linodo.i_block[i] == -1){
            continue;
        }

        for(int z = 0; z < 16; z++){
            eapuntador.b_pointers[z] = -1;
            eapuntador_doble.b_pointers[z] = -1;
            eapuntador_triple.b_pointers[z] = -1;
        }

        for(int z = 0; z < 4; z++){
            strcpy(ecarpeta.b_content[z].b_name, "-");
            ecarpeta.b_content[z].b_inodo = -1;
        }
        
        if(i == 12){
            //Recorrer el bloque de apuntadores simple
            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * linodo.i_block[i]);
            fseek(archivo, posLectura, SEEK_SET);
            fread(&lapuntador, sizeof(bapuntadores), 1, archivo);

            //Buscar un bloque
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
                    return -1;
                }
            }

            //Recorrerlo
            for(int j = 0; j < 16; j++){
                if(lapuntador.b_pointers[j] == -1){
                    continue;
                }

                posLectura = sblock.s_block_start + (sizeof(bcarpetas) * lapuntador.b_pointers[j]);
                fseek(archivo, posLectura, SEEK_SET);
                fread(&lcarpeta, sizeof(barchivos), 1, archivo);

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
                        return -1;
                    }
                }

                //Llenar la carpeta
                for(int k = 0; k < 4; k++){
                    std::string carpeta(lcarpeta.b_content[k].b_name);

                    if(carpeta == "-"){
                        continue;
                    }

                    int inodo_revisar = -1;
                    //Leer el inodo
                    posLectura = sblock.s_inode_start + (sizeof(inodo) * lcarpeta.b_content[k].b_inodo);
                    fseek(archivo, posLectura, SEEK_SET);
                    fread(&tinodo, sizeof(inodo), 1, archivo);

                    if(tinodo.i_type == '1'){
                        inodo_revisar = archivos(sesion, sblock, ruta, lcarpeta.b_content[k].b_inodo); 
                    }else{
                        inodo_revisar = carpetas(sesion, sblock, ruta, lcarpeta.b_content[k].b_inodo, no_inodo);
                    }

                    if(inodo_revisar == -1){
                        continue;
                    }

                    strcpy(ecarpeta.b_content[k].b_name, lcarpeta.b_content[k].b_name);
                    ecarpeta.b_content[k].b_inodo = inodo_revisar;
                }
                
                //Revisar que el bloque no esté vacío
                int contador = 0;
                for(int z = 0; z < 4; z++){
                    if(ecarpeta.b_content[z].b_name != "-"){
                        contador += 1;
                    }
                }

                if(contador == 0){
                    continue;
                }
                //Actualizar el apuntador
                eapuntador.b_pointers[j] = bloque_usado;
                sblock.s_free_blocks_count -= 1;
            }

            //Escribir el bloque de apuntadores
            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * bloque_apuntadors);
            fseek(archivo, posLectura, SEEK_SET);
            fwrite(&eapuntador, sizeof(bapuntadores), 1, archivo);

            //Actualizar el inodo
            cinodo.i_block[i] = bloque_apuntadors;
            sblock.s_free_blocks_count -= 1;

        }else if(i == 13){
            //Recorrer el bloque de apuntadores doble
            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * linodo.i_block[i]);
            fseek(archivo, posLectura, SEEK_SET);
            fread(&lapuntador_doble, sizeof(bapuntadores), 1, archivo);

            //Escribir en el bitmap el bloque de apuntadores doble
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
                    return -1;
                }
            }

            //Recorrer el inodo doble
            for(int j = 0; j < 16; j++){
                if(lapuntador_doble.b_pointers[j] == -1){
                    continue;
                }

                posLectura = sblock.s_block_start + (sizeof(bapuntadores) * lapuntador_doble.b_pointers[j]);
                fseek(archivo, posLectura, SEEK_SET);
                fread(&lapuntador, sizeof(bapuntadores), 1, archivo);

                //Buscar un bloque
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
                        return -1;
                    }
                }

                //Recorrerlo
                for(int k = 0; k < 16; k++){
                    if(lapuntador.b_pointers[k] == -1){
                        continue;
                    }

                    posLectura = sblock.s_block_start + (sizeof(bcarpetas) * lapuntador.b_pointers[k]);
                    fseek(archivo, posLectura, SEEK_SET);
                    fread(&lcarpeta, sizeof(barchivos), 1, archivo);

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
                            return -1;
                        }
                    }

                    //Llenar la carpeta
                    for(int l = 0; l < 4; l++){
                        std::string carpeta(lcarpeta.b_content[l].b_name);

                        if(carpeta == "-"){
                            continue;
                        }

                        int inodo_revisar = -1;
                        //Leer el inodo
                        posLectura = sblock.s_inode_start + (sizeof(inodo) * lcarpeta.b_content[l].b_inodo);
                        fseek(archivo, posLectura, SEEK_SET);
                        fread(&tinodo, sizeof(inodo), 1, archivo);

                        if(tinodo.i_type == '1'){
                            inodo_revisar = archivos(sesion, sblock, ruta, lcarpeta.b_content[l].b_inodo); 
                        }else{
                            inodo_revisar = carpetas(sesion, sblock, ruta, lcarpeta.b_content[l].b_inodo, no_inodo);
                        }

                        if(inodo_revisar == -1){
                            continue;
                        }

                        strcpy(ecarpeta.b_content[l].b_name, lcarpeta.b_content[k].b_name);
                        ecarpeta.b_content[l].b_inodo = inodo_revisar;
                    }
                    
                    //Revisar que el bloque no esté vacío
                    int contador = 0;
                    for(int z = 0; z < 4; z++){
                        if(ecarpeta.b_content[z].b_name != "-"){
                            contador += 1;
                        }
                    }

                    if(contador == 0){
                        continue;
                    }
                    //Actualizar el apuntador
                    eapuntador.b_pointers[k] = bloque_usado;
                    sblock.s_free_blocks_count -= 1;
                }

                //Escribir el bloque de apuntadores
                posLectura = sblock.s_block_start + (sizeof(bapuntadores) * bloque_apuntadors);
                fseek(archivo, posLectura, SEEK_SET);
                fwrite(&eapuntador, sizeof(bapuntadores), 1, archivo);

                //Actualizar el apuntador
                eapuntador_doble.b_pointers[j] = bloque_apuntadors;
                sblock.s_free_blocks_count -= 1;
            }

            //Escribir el bloque de apuntadores
            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * bloque_apuntadord);
            fseek(archivo, posLectura, SEEK_SET);
            fwrite(&eapuntador_doble, sizeof(bapuntadores), 1, archivo);

            //Actualizar el inodo
            cinodo.i_block[i] = bloque_apuntadord;
            sblock.s_free_blocks_count -= 1;

        }else if(i == 14){
            //Recorrer el bloque de apuntadores triple
            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * linodo.i_block[i]);
            fseek(archivo, posLectura, SEEK_SET);
            fread(&lapuntador_triple, sizeof(bapuntadores), 1, archivo);

            //Escribir en el bitmap el bloque de apuntadores triple
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
                    return -1;
                }
            }

            //Recorrerlo
            for(int j = 0; j < 16; j++){
                if(lapuntador_triple.b_pointers[j] == -1){
                    continue;
                }

                posLectura = sblock.s_block_start + (sizeof(bapuntadores) * lapuntador_triple.b_pointers[j]);
                fseek(archivo, posLectura, SEEK_SET);
                fread(&lapuntador_doble, sizeof(bapuntadores), 1, archivo);

                //Buscar un bloque
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
                        return -1;
                    }
                }

                //Recorrerlo
                for(int k = 0; k < 16; k++){
                    if(lapuntador_doble.b_pointers[k] == -1){
                        continue;
                    }
                    
                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * lapuntador_doble.b_pointers[k]);
                    fseek(archivo, posLectura, SEEK_SET);
                    fread(&lapuntador, sizeof(bapuntadores), 1, archivo);

                    //Buscar un bloque
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
                            return -1;
                        }
                    }

                    //Recorrerlo
                    for(int l = 0; l < 16; l++){
                        if(lapuntador.b_pointers[l] == -1){
                            continue;
                        }
                        
                        posLectura = sblock.s_block_start + (sizeof(bcarpetas) * lapuntador.b_pointers[l]);
                        fseek(archivo, posLectura, SEEK_SET);
                        fread(&lcarpeta, sizeof(barchivos), 1, archivo);

                        //Buscar un bloque
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
                                return -1;
                            }
                        }

                        //Llenar la carpeta
                        for(int m = 0; m < 4; m++){
                            std::string carpeta(lcarpeta.b_content[m].b_name);

                            if(carpeta == "-"){
                                continue;
                            }

                            int inodo_revisar = -1;
                            //Leer el inodo
                            posLectura = sblock.s_inode_start + (sizeof(inodo) * lcarpeta.b_content[m].b_inodo);
                            fseek(archivo, posLectura, SEEK_SET);
                            fread(&tinodo, sizeof(inodo), 1, archivo);

                            if(tinodo.i_type == '1'){
                                inodo_revisar = archivos(sesion, sblock, ruta, lcarpeta.b_content[m].b_inodo); 
                            }else{
                                inodo_revisar = carpetas(sesion, sblock, ruta, lcarpeta.b_content[m].b_inodo, no_inodo);
                            }

                            if(inodo_revisar == -1){
                                continue;
                            }

                            strcpy(ecarpeta.b_content[m].b_name, lcarpeta.b_content[m].b_name);
                            ecarpeta.b_content[m].b_inodo = inodo_revisar;
                        }
                        
                        //Revisar que el bloque no esté vacío
                        int contador = 0;
                        for(int z = 0; z < 4; z++){
                            if(ecarpeta.b_content[z].b_name != "-"){
                                contador += 1;
                            }
                        }

                        if(contador == 0){
                            continue;
                        }
                        //Actualizar el apuntador
                        eapuntador.b_pointers[l] = bloque_usado;
                        sblock.s_free_blocks_count -= 1;
                   
                    }
                
                    //Escribir el bloque de apuntadores
                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * bloque_apuntadors);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&eapuntador, sizeof(bapuntadores), 1, archivo);

                    //Actualizar el apuntador
                    eapuntador_doble.b_pointers[k] = bloque_apuntadors;
                    sblock.s_free_blocks_count -= 1;
                }
            
                //Escribir el bloque de apuntadores
                posLectura = sblock.s_block_start + (sizeof(bapuntadores) * bloque_apuntadord);
                fseek(archivo, posLectura, SEEK_SET);
                fwrite(&eapuntador_doble, sizeof(bapuntadores), 1, archivo);

                //Actualizar el apuntador
                eapuntador_triple.b_pointers[j] = bloque_apuntadord;
                sblock.s_free_blocks_count -= 1;
            }

            //Escribir el bloque de apuntadores
            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * bloque_apuntadort);
            fseek(archivo, posLectura, SEEK_SET);
            fwrite(&eapuntador_triple, sizeof(bapuntadores), 1, archivo);

            //Actualizar el inodo
            cinodo.i_block[i] = bloque_apuntadort;
            sblock.s_free_blocks_count -= 1;

        }else if(i == 0){
            posLectura = sblock.s_block_start + (sizeof(bcarpetas) * linodo.i_block[i]);
            fseek(archivo, posLectura, SEEK_SET);
            fread(&lcarpeta, sizeof(bcarpetas), 1, archivo);

            //Buscar el espacio
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
                    return -1;
                }
            }

            //Añadir al bloque el padre y la actual
            strcpy(ecarpeta.b_content[0].b_name, ".");
            ecarpeta.b_content[0].b_inodo = no_inodo;

            strcpy(ecarpeta.b_content[1].b_name, "..");
            ecarpeta.b_content[1].b_inodo = padre_inodo;


            //Llenar la carpeta
            for(int j = 2; j < 4; j++){
                std::string carpeta(lcarpeta.b_content[j].b_name);

                if(carpeta == "-"){
                    continue;
                }

                int inodo_revisar = -1;
                //Leer el inodo
                posLectura = sblock.s_inode_start + (sizeof(inodo) * lcarpeta.b_content[j].b_inodo);
                fseek(archivo, posLectura, SEEK_SET);
                fread(&tinodo, sizeof(inodo), 1, archivo);

                if(tinodo.i_type == '1'){
                    inodo_revisar = archivos(sesion, sblock, ruta, lcarpeta.b_content[j].b_inodo); 
                }else{
                    inodo_revisar = carpetas(sesion, sblock, ruta, lcarpeta.b_content[j].b_inodo, no_inodo);
                }

                if(inodo_revisar == -1){
                    continue;
                }

                strcpy(ecarpeta.b_content[j].b_name, lcarpeta.b_content[j].b_name);
                ecarpeta.b_content[j].b_inodo = inodo_revisar;
            }
            
            //Revisar que el bloque no esté vacío
            int contador = 0;
            for(int z = 0; z < 4; z++){
                if(lcarpeta.b_content[z].b_name != "-"){
                    contador += 1;
                }
            }

            if(contador == 0){
                continue;
            }

            //Escribir el bloque
            posLectura = sblock.s_block_start + (sizeof(bcarpetas) * bloque_usado);
            fseek(archivo, posLectura, SEEK_SET);
            fwrite(&ecarpeta, sizeof(bcarpetas), 1, archivo);

            //Actualizar el inodo
            cinodo.i_block[i] = bloque_usado;
            sblock.s_free_blocks_count -= 1;
        
        }else{
            posLectura = sblock.s_block_start + (sizeof(bcarpetas) * linodo.i_block[i]);
            fseek(archivo, posLectura, SEEK_SET);
            fread(&lcarpeta, sizeof(bcarpetas), 1, archivo);

            //Buscar el espacio
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
                    return -1;
                }
            }

            //Llenar la carpeta
            for(int j = 0; j < 4; j++){
                std::string carpeta(lcarpeta.b_content[j].b_name);

                if(carpeta == "-"){
                    continue;
                }

                int inodo_revisar = -1;
                //Leer el inodo
                posLectura = sblock.s_inode_start + (sizeof(inodo) * lcarpeta.b_content[j].b_inodo);
                fseek(archivo, posLectura, SEEK_SET);
                fread(&tinodo, sizeof(inodo), 1, archivo);

                if(tinodo.i_type == '1'){
                    inodo_revisar = archivos(sesion, sblock, ruta, lcarpeta.b_content[j].b_inodo); 
                }else{
                    inodo_revisar = carpetas(sesion, sblock, ruta, lcarpeta.b_content[j].b_inodo, no_inodo);
                }

                if(inodo_revisar == -1){
                    continue;
                }

                strcpy(ecarpeta.b_content[j].b_name, lcarpeta.b_content[j].b_name);
                ecarpeta.b_content[j].b_inodo = inodo_revisar;
            }
            
            //Revisar que el bloque no esté vacío
            int contador = 0;
            for(int z = 0; z < 4; z++){
                if(ecarpeta.b_content[z].b_name != "-"){
                    contador += 1;
                }
            }

            if(contador == 0){
                continue;
            }

            //Escribir el bloque
            posLectura = sblock.s_block_start + (sizeof(bcarpetas) * bloque_usado);
            fseek(archivo, posLectura, SEEK_SET);
            fwrite(&ecarpeta, sizeof(bcarpetas), 1, archivo);

            //Actualizar el inodo
            cinodo.i_block[i] = bloque_usado;
            sblock.s_free_blocks_count -= 1;
        }
    }
    
    //ESCRIBIR EL INODO
    cinodo.i_mtime = time(NULL);
    posLectura = sblock.s_inode_start + (sizeof(inodo) * ninodo);
    fseek(archivo, posLectura, SEEK_SET);
    fwrite(&cinodo, sizeof(inodo), 1, archivo);

    fclose(archivo);
    return ninodo;
}

int archivos(usuario &sesion, sbloque &sblock, std::string &ruta, int &no_inodo){
    //VARIABLES
    FILE *archivo;                             //Sirve para verificar que el archivo exista
    int posLectura;                            //Para determinar la posicion de lectura en disco
    inodo linodo;                              //Para el manejo de los inodos
    inodo cinodo;                              //Inodo nuevo que se va a copiar
    barchivos larchivo;                        //Para el manejo de bloques de archivo
    bapuntadores lapuntador;                   //Para el manejo de bloques de apuntadores simples
    bapuntadores lapuntador_doble;             //Para el manejo de bloques de apuntadores dobles
    bapuntadores lapuntador_triple;            //Para el manejo de bloques de apuntadores triples
    bool continuar = true;                     //Usado como bandera al buscar la ruta
    int ninodo = -1;                            //Número de Inodo que se va a usar
    char c;

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
        if(permisos[0] == '4' || permisos[0] == '5' || permisos[0] == '6' || permisos[0] == '7'){
            acceso = true;
        }
    }else if(ugo == 2){
        if(permisos[1] == '4' || permisos[1] == '5' || permisos[1] == '6' || permisos[1] == '7'){
            acceso = true;
        }
    }else{
        if(permisos[2] == '4' || permisos[2] == '5' || permisos[2] == '6' || permisos[2] == '7'){
            acceso = true;
        }
    }

    if(sesion.user == "root"){
        acceso = true;
    }

    if(!acceso){
        std::cout << "ERROR: No posee permisos para copiar el archivo." << std::endl;
        fclose(archivo);
        return ninodo;
    }

    //COPIAR EL INODO
    //Buscar un espacio
    for(int a = 0; a < sblock.s_inodes_count; a++){
        posLectura = sblock.s_bm_inode_start + (sizeof(char) * a);
        fseek(archivo, posLectura, SEEK_SET);
        fread(&c, sizeof(char), 1, archivo);

        if(c == '\0'){
            ninodo = a;
            c = '1';
            fseek(archivo, posLectura, SEEK_SET);
            fwrite(&c ,sizeof(char), 1 ,archivo);
            break;
        }

        if(a == sblock.s_inodes_count - 1){
            std::cout << "ERROR: No hay inodos disponibles." << std::endl;
            fclose(archivo);
            return ninodo;
        }
    }
    
    //Escribir sus datos
    cinodo.i_uid = linodo.i_uid;
    cinodo.i_gid = linodo.i_gid;
    cinodo.i_s = linodo.i_s;
    cinodo.i_atime = linodo.i_atime;
    cinodo.i_ctime = linodo.i_ctime;
    cinodo.i_mtime = linodo.i_mtime;
    for(int i = 0; i < 15; i++){
        cinodo.i_block[i] = -1;
    }
    cinodo.i_type = linodo.i_type;
    cinodo.i_perm = linodo.i_perm;
    
    //Copiar el contenido
    for(int i = 0; i < 15; i++){
        int bloque_usado = -1;
        int bloque_apuntadors = -1;
        int bloque_apuntadord = -1;
        int bloque_apuntadort = -1;
        barchivos earchivo;
        bapuntadores eapuntador;
        bapuntadores eapuntador_doble;
        bapuntadores eapuntador_triple;

        if(linodo.i_block[i] == -1){
            continue;
        }

        for(int z = 0; z < 16; z++){
            eapuntador.b_pointers[z] = -1;
            eapuntador_doble.b_pointers[z] = -1;
            eapuntador_triple.b_pointers[z] = -1;
        }

        if(i == 12){
            //Recorrer el bloque de apuntadores simple
            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * linodo.i_block[i]);
            fseek(archivo, posLectura, SEEK_SET);
            fread(&lapuntador, sizeof(bapuntadores), 1, archivo);

            //Buscar un bloque
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
                    return -1;
                }
            }

            //Recorrerlo
            for(int j = 0; j < 16; j++){
                if(lapuntador.b_pointers[j] == -1){
                    continue;
                }

                posLectura = sblock.s_block_start + (sizeof(barchivos) * lapuntador.b_pointers[j]);
                fseek(archivo, posLectura, SEEK_SET);
                fread(&larchivo, sizeof(barchivos), 1, archivo);

                for(int a = 0; a < sblock.s_blocks_count; a++){
                    posLectura = sblock.s_bm_block_start + (sizeof(char) * a);
                    fseek(archivo, posLectura, SEEK_SET);
                    fread(&c, sizeof(char), 1, archivo);

                    if(c == '\0'){
                        bloque_usado = a;
                        c = 'a';
                        fseek(archivo, posLectura, SEEK_SET);
                        fwrite(&c ,sizeof(char), 1 ,archivo);
                        break;
                    }

                    if(a == sblock.s_blocks_count - 1){
                        std::cout << "ERROR: No hay bloques disponibles." << std::endl;
                        fclose(archivo);
                        return -1;
                    }
                }

                //Crear y escribir el bloque
                strcpy(earchivo.b_content, larchivo.b_content);
                posLectura = sblock.s_block_start + (sizeof(barchivos) * bloque_usado);
                fseek(archivo, posLectura, SEEK_SET);
                fwrite(&earchivo, sizeof(barchivos), 1, archivo);

                //Actualizar el apuntador
                eapuntador.b_pointers[j] = bloque_usado;
                sblock.s_free_blocks_count -= 1;
            }

            //Escribir el bloque de apuntadores
            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * bloque_apuntadors);
            fseek(archivo, posLectura, SEEK_SET);
            fwrite(&eapuntador, sizeof(bapuntadores), 1, archivo);

            //Actualizar el inodo
            cinodo.i_block[i] = bloque_apuntadors;
            sblock.s_free_blocks_count -= 1;

        }else if(i == 13){
            //Recorrer el bloque de apuntadores doble
            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * linodo.i_block[i]);
            fseek(archivo, posLectura, SEEK_SET);
            fread(&lapuntador_doble, sizeof(bapuntadores), 1, archivo);

            //Escribir en el bitmap el bloque de apuntadores doble
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
                    return -1;
                }
            }

            //Recorrer el inodo doble
            for(int j = 0; j < 16; j++){
                if(lapuntador_doble.b_pointers[j] == -1){
                    continue;
                }

                posLectura = sblock.s_block_start + (sizeof(bapuntadores) * lapuntador_doble.b_pointers[j]);
                fseek(archivo, posLectura, SEEK_SET);
                fread(&lapuntador, sizeof(bapuntadores), 1, archivo);

                //Buscar un bloque
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
                        return -1;
                    }
                }

                //Recorrerlo
                for(int k = 0; k < 16; k++){
                    if(lapuntador.b_pointers[k] == -1){
                        continue;
                    }

                    posLectura = sblock.s_block_start + (sizeof(barchivos) * lapuntador.b_pointers[k]);
                    fseek(archivo, posLectura, SEEK_SET);
                    fread(&larchivo, sizeof(barchivos), 1, archivo);

                    for(int a = 0; a < sblock.s_blocks_count; a++){
                        posLectura = sblock.s_bm_block_start + (sizeof(char) * a);
                        fseek(archivo, posLectura, SEEK_SET);
                        fread(&c, sizeof(char), 1, archivo);

                        if(c == '\0'){
                            bloque_usado = a;
                            c = 'a';
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&c ,sizeof(char), 1 ,archivo);
                            break;
                        }

                        if(a == sblock.s_blocks_count - 1){
                            std::cout << "ERROR: No hay bloques disponibles." << std::endl;
                            fclose(archivo);
                            return -1;
                        }
                    }

                    //Crear y escribir el bloque
                    strcpy(earchivo.b_content, larchivo.b_content);
                    posLectura = sblock.s_block_start + (sizeof(barchivos) * bloque_usado);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&earchivo, sizeof(barchivos), 1, archivo);

                    //Actualizar el apuntador
                    eapuntador.b_pointers[k] = bloque_usado;
                    sblock.s_free_blocks_count -= 1;
                }

                //Escribir el bloque de apuntadores
                posLectura = sblock.s_block_start + (sizeof(bapuntadores) * bloque_apuntadors);
                fseek(archivo, posLectura, SEEK_SET);
                fwrite(&eapuntador, sizeof(bapuntadores), 1, archivo);

                //Actualizar el apuntador
                eapuntador_doble.b_pointers[j] = bloque_apuntadors;
                sblock.s_free_blocks_count -= 1;
            }

            //Escribir el bloque de apuntadores
            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * bloque_apuntadord);
            fseek(archivo, posLectura, SEEK_SET);
            fwrite(&eapuntador_doble, sizeof(bapuntadores), 1, archivo);

            //Actualizar el inodo
            cinodo.i_block[i] = bloque_apuntadord;
            sblock.s_free_blocks_count -= 1;

        }else if(i == 14){
            //Recorrer el bloque de apuntadores triple
            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * linodo.i_block[i]);
            fseek(archivo, posLectura, SEEK_SET);
            fread(&lapuntador_triple, sizeof(bapuntadores), 1, archivo);

            //Escribir en el bitmap el bloque de apuntadores triple
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
                    return -1;
                }
            }

            //Recorrerlo
            for(int j = 0; j < 16; j++){
                if(lapuntador_triple.b_pointers[j] == -1){
                    continue;
                }

                posLectura = sblock.s_block_start + (sizeof(bapuntadores) * lapuntador_triple.b_pointers[j]);
                fseek(archivo, posLectura, SEEK_SET);
                fread(&lapuntador_doble, sizeof(bapuntadores), 1, archivo);

                //Buscar un bloque
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
                        return -1;
                    }
                }

                //Recorrerlo
                for(int k = 0; k < 16; k++){
                    if(lapuntador_doble.b_pointers[k] == -1){
                        continue;
                    }
                    
                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * lapuntador_doble.b_pointers[k]);
                    fseek(archivo, posLectura, SEEK_SET);
                    fread(&lapuntador, sizeof(bapuntadores), 1, archivo);

                    //Buscar un bloque
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
                            return -1;
                        }
                    }

                    //Recorrerlo
                    for(int l = 0; l < 16; l++){
                        if(lapuntador.b_pointers[l] == -1){
                            continue;
                        }
                        
                        posLectura = sblock.s_block_start + (sizeof(barchivos) * lapuntador.b_pointers[l]);
                        fseek(archivo, posLectura, SEEK_SET);
                        fread(&larchivo, sizeof(barchivos), 1, archivo);

                        //Buscar un bloque
                        for(int a = 0; a < sblock.s_blocks_count; a++){
                            posLectura = sblock.s_bm_block_start + (sizeof(char) * a);
                            fseek(archivo, posLectura, SEEK_SET);
                            fread(&c, sizeof(char), 1, archivo);

                            if(c == '\0'){
                                bloque_usado = a;
                                c = 'a';
                                fseek(archivo, posLectura, SEEK_SET);
                                fwrite(&c ,sizeof(char), 1 ,archivo);
                                break;
                            }

                            if(a == sblock.s_blocks_count - 1){
                                std::cout << "ERROR: No hay bloques disponibles." << std::endl;
                                fclose(archivo);
                                return -1;
                            }
                        }

                        //Crear y escribir el bloque
                        strcpy(earchivo.b_content, larchivo.b_content);
                        posLectura = sblock.s_block_start + (sizeof(barchivos) * bloque_usado);
                        fseek(archivo, posLectura, SEEK_SET);
                        fwrite(&earchivo, sizeof(barchivos), 1, archivo);

                        //Actualizar el apuntador
                        eapuntador.b_pointers[l] = bloque_usado;
                        sblock.s_free_blocks_count -= 1;
                   
                    }
                
                    //Escribir el bloque de apuntadores
                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * bloque_apuntadors);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&eapuntador, sizeof(bapuntadores), 1, archivo);

                    //Actualizar el apuntador
                    eapuntador_doble.b_pointers[k] = bloque_apuntadors;
                    sblock.s_free_blocks_count -= 1;
                }
            
                //Escribir el bloque de apuntadores
                posLectura = sblock.s_block_start + (sizeof(bapuntadores) * bloque_apuntadord);
                fseek(archivo, posLectura, SEEK_SET);
                fwrite(&eapuntador_doble, sizeof(bapuntadores), 1, archivo);

                //Actualizar el apuntador
                eapuntador_triple.b_pointers[j] = bloque_apuntadord;
                sblock.s_free_blocks_count -= 1;
            }

            //Escribir el bloque de apuntadores
            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * bloque_apuntadort);
            fseek(archivo, posLectura, SEEK_SET);
            fwrite(&eapuntador_triple, sizeof(bapuntadores), 1, archivo);

            //Actualizar el inodo
            cinodo.i_block[i] = bloque_apuntadort;
            sblock.s_free_blocks_count -= 1;

        }else{
            posLectura = sblock.s_block_start + (sizeof(barchivos) * linodo.i_block[i]);
            fseek(archivo, posLectura, SEEK_SET);
            fread(&larchivo, sizeof(barchivos), 1, archivo);

            //Buscar el espacio
            for(int a = 0; a < sblock.s_blocks_count; a++){
                posLectura = sblock.s_bm_block_start + (sizeof(char) * a);
                fseek(archivo, posLectura, SEEK_SET);
                fread(&c, sizeof(char), 1, archivo);

                if(c == '\0'){
                    bloque_usado = a;
                    c = 'a';
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&c ,sizeof(char), 1 ,archivo);
                    break;
                }

                if(a == sblock.s_blocks_count - 1){
                    std::cout << "ERROR: No hay bloques disponibles." << std::endl;
                    fclose(archivo);
                    return -1;
                }
            }

            //Crear y escribir el bloque
            strcpy(earchivo.b_content, larchivo.b_content);
            posLectura = sblock.s_block_start + (sizeof(barchivos) * bloque_usado);
            fseek(archivo, posLectura, SEEK_SET);
            fwrite(&earchivo, sizeof(barchivos), 1, archivo);

            //Actualizar el inodo
            cinodo.i_block[i] = bloque_usado;
            sblock.s_free_blocks_count -= 1;
        }
    }
    
    //ESCRIBIR EL INODO
    cinodo.i_mtime = time(NULL);
    posLectura = sblock.s_inode_start + (sizeof(inodo) * ninodo);
    fseek(archivo, posLectura, SEEK_SET);
    fwrite(&cinodo, sizeof(inodo), 1, archivo);

    fclose(archivo);
    return ninodo;
}
