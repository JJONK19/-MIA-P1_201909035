#include "find.h"

void find(std::vector<std::string> &parametros, std::vector<disco> &discos, usuario &sesion){
    
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
    inodo tinodo;                              //Para el manejo de los inodos de las carpetas 
    bcarpetas lcarpeta;                        //Para el manejo de bloques de carpetas
    barchivos larchivo;                        //Para el manejo de bloques de archivo
    bapuntadores lapuntador;                   //Para el manejo de bloques de apuntadores simples
    bapuntadores lapuntador_doble;             //Para el manejo de bloques de apuntadores dobles
    bapuntadores lapuntador_triple;            //Para el manejo de bloques de apuntadores triples
    bool continuar = true;                     //Usado como bandera al buscar la ruta
    std::string nombre_archivo;                //Nombre del archivo sin la ruta
    int inodo_leer = -1;                       //Inodo que se va a copiar
    std::vector<std::string> carpetas;         //Nombre de las carpetas usado para imprimir la ruta


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
    if(ruta == "" || nombre == ""){
        required = false;
    }

    if(!required){
        std::cout << "ERROR: La instrucción find carece de todos los parametros obligatorios." << std::endl;
    }

    //PREPARACIÓN DE PARAMETROS - Extraer de la ruta el nombre del archivo 
    nombre_archivo = ruta.substr(ruta.find_last_of("\\/") + 1, ruta.size() - 1);
    
    //PREPARACIÓN DE PARAMETROS - Separar los nombres que vengan en las rutas.
    std::regex separador("/");
    std::vector<std::string> path(std::sregex_token_iterator(ruta.begin(), ruta.end(), separador, -1),
                    std::sregex_token_iterator());

    //PREPARACION DE PARAMETROS - Preparar la regex sustituyendo los caracteres especiales
    std::string nombre_regex = "";
    for(int i = 0; i < nombre.size(); i++){
        if(nombre[i] == '^'){
            nombre_regex.append("\\^");
        }else if(nombre[i] == '$'){
            nombre_regex.append("\\$");
        }else if(nombre[i] == '\\'){
            nombre_regex.append("\\\\");
        }else if(nombre[i] == '.'){
            nombre_regex.append("\\.");
        }else if(nombre[i] == '*'){
            nombre_regex.append(".*");
        }else if(nombre[i] == '+'){
            nombre_regex.append("\\+");
        }else if(nombre[i] == '?'){
            nombre_regex.append(".?");
        }else if(nombre[i] == '('){
            nombre_regex.append("\\(");
        }else if(nombre[i] == ')'){
            nombre_regex.append("\\)");
        }else if(nombre[i] == '['){
            nombre_regex.append("\\[");
        }else if(nombre[i] == ']'){
            nombre_regex.append("\\]");
        }else if(nombre[i] == '{'){
            nombre_regex.append("\\{");
        }else if(nombre[i] == '}'){
            nombre_regex.append("\\}");
        }else if(nombre[i] == '|'){
            nombre_regex.append("\\|");
        }else{
            nombre_regex.push_back(nombre[i]);
        }
    }

    std::regex rnombre(nombre_regex);
                
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

    //BUSCAR LA CARPETA 
    posicion = 1;
    continuar = true;
    if(ruta == "/"){
        inodo_leer = 0;
        continuar = false;
    }else if(path[0] != "\0"){
        std::cout << "ERROR: La ruta ingresada es erronea." << std::endl;
        fclose(archivo);
        return;
    }

    while(continuar){
        int inodo_temporal = -1;
        
        for(int i = 0; i < 15; i++){
            if(inodo_leer != -1 || inodo_temporal != -1){
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
                                inodo_leer = lcarpeta.b_content[k].b_inodo;
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
                                    inodo_leer = lcarpeta.b_content[l].b_inodo;
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
                                        inodo_leer = lcarpeta.b_content[m].b_inodo;
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
                            inodo_leer = lcarpeta.b_content[j].b_inodo;
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
        }else if(posicion == path.size() && inodo_leer == -1){
            continuar = false;
            std::cout << "ERROR: La ruta del contenido es erronea." << std::endl;
            fclose(archivo);
            return;
        }
    }
    
    //LEER INODO DEL CONTENIDO
    posLectura = sblock.s_inode_start + (sizeof(inodo) * inodo_leer);
    fseek(archivo, posLectura, SEEK_SET);
    fread(&linodo, sizeof(inodo), 1, archivo);
    
    //VERIFICAR QUE SEA UNA CARPETA
    if(linodo.i_type == '1'){
        std::cout << "ERROR: El destino debe de ser una carpeta." << std::endl;
        fclose(archivo);
        return; 
    }

    //VERIFICAR QUE POSEA PERMISO PARA LEER 
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
        std::cout << "ERROR: No posee permisos para escribir en la carpeta destino." << std::endl;
        fclose(archivo);
        return;
    }
    
    //BUSCAR COINCIDENCIAS
    //Insertar la "raiz"
    if(ruta != "/"){
        carpetas.push_back(nombre_archivo);   
    }

    //Recorrer las carpetas
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

                    if(carpeta == "-"){
                        continue;
                    }

                    //Verificar si cumple el nombre
                    if(std::regex_match(carpeta, rnombre)){
                        std::string imprimir = "";
                        for(int z = 0; z < carpetas.size(); z++){
                            if(z == 0){
                                imprimir.append("./");
                            }else{
                                imprimir.append("/");
                            }
                            imprimir.append(carpetas[z]);
                        }
                        imprimir.append("/");
                        imprimir.append(carpeta);
                        std::cout << imprimir << std::endl;
                    }

                    //Leer el inodo       
                    posLectura = sblock.s_inode_start + (sizeof(inodo) * lcarpeta.b_content[j].b_inodo);
                    fseek(archivo, posLectura, SEEK_SET);
                    fread(&tinodo, sizeof(inodo), 1, archivo);

                    //Verificar si es carpeta para revisar recursivamente
                    if(tinodo.i_type == '0'){
                        carpetas.push_back(carpeta);
                        buscar(sesion, sblock, tempD.ruta, lcarpeta.b_content[j].b_inodo, carpetas, rnombre);
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

                        if(carpeta == "-"){
                            continue;
                        }

                        //Verificar si cumple el nombre
                        if(std::regex_match(carpeta, rnombre)){
                            std::string imprimir = "";
                            for(int z = 0; z < carpetas.size(); z++){
                                if(z == 0){
                                    imprimir.append("./");
                                }else{
                                    imprimir.append("/");
                                }
                                imprimir.append(carpetas[z]);
                            }
                            imprimir.append("/");
                            imprimir.append(carpeta);
                            std::cout << imprimir << std::endl;
                        }

                        //Leer el inodo       
                        posLectura = sblock.s_inode_start + (sizeof(inodo) * lcarpeta.b_content[j].b_inodo);
                        fseek(archivo, posLectura, SEEK_SET);
                        fread(&tinodo, sizeof(inodo), 1, archivo);

                        //Verificar si es carpeta para revisar recursivamente
                        if(tinodo.i_type == '0'){
                            carpetas.push_back(carpeta);
                            buscar(sesion, sblock, tempD.ruta, lcarpeta.b_content[j].b_inodo, carpetas, rnombre);
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
                                continue;
                            }

                            //Verificar si cumple el nombre
                            if(std::regex_match(carpeta, rnombre)){
                                std::string imprimir = "";
                                for(int z = 0; z < carpetas.size(); z++){
                                    if(z == 0){
                                        imprimir.append("./");
                                    }else{
                                        imprimir.append("/");
                                    }
                                    imprimir.append(carpetas[z]);
                                }
                                imprimir.append("/");
                                imprimir.append(carpeta);
                                std::cout << imprimir << std::endl;
                            }

                            //Leer el inodo       
                            posLectura = sblock.s_inode_start + (sizeof(inodo) * lcarpeta.b_content[j].b_inodo);
                            fseek(archivo, posLectura, SEEK_SET);
                            fread(&tinodo, sizeof(inodo), 1, archivo);

                            //Verificar si es carpeta para revisar recursivamente
                            if(tinodo.i_type == '0'){
                                carpetas.push_back(carpeta);
                                buscar(sesion, sblock, tempD.ruta, lcarpeta.b_content[j].b_inodo, carpetas, rnombre);
                            }
                        }
                    }
                }
            }
        }else if(i == 0){
            posLectura = sblock.s_block_start + (sizeof(bcarpetas) * linodo.i_block[i]);
            fseek(archivo, posLectura, SEEK_SET);
            fread(&lcarpeta, sizeof(bcarpetas), 1, archivo);

            for(int j = 2; j < 4; j++){
                std::string carpeta(lcarpeta.b_content[j].b_name);

                if(carpeta == "-"){
                    continue;
                }

                //Verificar si cumple el nombre
                if(std::regex_match(carpeta, rnombre)){
                    std::string imprimir = "";
                    for(int z = 0; z < carpetas.size(); z++){
                        if(z == 0){
                            imprimir.append("./");
                        }else{
                            imprimir.append("/");
                        }
                        imprimir.append(carpetas[z]);
                    }
                    imprimir.append("/");
                    imprimir.append(carpeta);
                    std::cout << imprimir << std::endl;
                }

                //Leer el inodo       
                posLectura = sblock.s_inode_start + (sizeof(inodo) * lcarpeta.b_content[j].b_inodo);
                fseek(archivo, posLectura, SEEK_SET);
                fread(&tinodo, sizeof(inodo), 1, archivo);

                //Verificar si es carpeta para revisar recursivamente
                if(tinodo.i_type == '0'){
                    carpetas.push_back(carpeta);
                    buscar(sesion, sblock, tempD.ruta, lcarpeta.b_content[j].b_inodo, carpetas, rnombre);
                }
            }
        }else{
            posLectura = sblock.s_block_start + (sizeof(bcarpetas) * linodo.i_block[i]);
            fseek(archivo, posLectura, SEEK_SET);
            fread(&lcarpeta, sizeof(bcarpetas), 1, archivo);

            for(int j = 0; j < 4; j++){
                std::string carpeta(lcarpeta.b_content[j].b_name);

                if(carpeta == "-"){
                    continue;
                }

                //Verificar si cumple el nombre
                if(std::regex_match(carpeta, rnombre)){
                    std::string imprimir = "";
                    for(int z = 0; z < carpetas.size(); z++){
                        if(z == 0){
                            imprimir.append("./");
                        }else{
                            imprimir.append("/");
                        }
                        imprimir.append(carpetas[z]);
                    }
                    imprimir.append("/");
                    imprimir.append(carpeta);
                    std::cout << imprimir << std::endl;
                }

                //Leer el inodo       
                posLectura = sblock.s_inode_start + (sizeof(inodo) * lcarpeta.b_content[j].b_inodo);
                fseek(archivo, posLectura, SEEK_SET);
                fread(&tinodo, sizeof(inodo), 1, archivo);

                //Verificar si es carpeta para revisar recursivamente
                if(tinodo.i_type == '0'){
                    carpetas.push_back(carpeta);
                    buscar(sesion, sblock, tempD.ruta, lcarpeta.b_content[j].b_inodo, carpetas, rnombre);
                }
            }
        }
    }

    //Actualizar Inodo
    archivo = fopen(tempD.ruta.c_str(), "r+b");
    linodo.i_atime = time(NULL);
    posLectura = sblock.s_inode_start + (sizeof(inodo) * inodo_leer);
    fseek(archivo, posLectura, SEEK_SET);
    fwrite(&linodo, sizeof(inodo), 1, archivo);

    fclose(archivo);
    std::cout << "MENSAJE: Busqueda finalizada." << std::endl;
}

void buscar(usuario &sesion, sbloque &sblock, std::string &ruta, int &no_inodo, std::vector<std::string> &carpetas, std::regex &nombre){
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
        return;
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

                    if(carpeta == "-"){
                        continue;
                    }

                    //Verificar si cumple el nombre
                    if(std::regex_match(carpeta, nombre)){
                        std::string imprimir = "";
                        for(int z = 0; z < carpetas.size(); z++){
                            if(z == 0){
                                imprimir.append("./");
                            }else{
                                imprimir.append("/");
                            }
                            imprimir.append(carpetas[z]);
                        }
                        imprimir.append("/");
                        imprimir.append(carpeta);
                        std::cout << imprimir << std::endl;
                    }

                    //Leer el inodo       
                    posLectura = sblock.s_inode_start + (sizeof(inodo) * lcarpeta.b_content[j].b_inodo);
                    fseek(archivo, posLectura, SEEK_SET);
                    fread(&tinodo, sizeof(inodo), 1, archivo);

                    //Verificar si es carpeta para revisar recursivamente
                    if(tinodo.i_type == '0'){
                        carpetas.push_back(carpeta);
                        buscar(sesion, sblock, ruta, lcarpeta.b_content[j].b_inodo, carpetas, nombre);
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

                        if(carpeta == "-"){
                            continue;
                        }

                        //Verificar si cumple el nombre
                        if(std::regex_match(carpeta, nombre)){
                            std::string imprimir = "";
                            for(int z = 0; z < carpetas.size(); z++){
                                if(z == 0){
                                    imprimir.append("./");
                                }else{
                                    imprimir.append("/");
                                }
                                imprimir.append(carpetas[z]);
                            }
                            imprimir.append("/");
                            imprimir.append(carpeta);
                            std::cout << imprimir << std::endl;
                        }

                        //Leer el inodo       
                        posLectura = sblock.s_inode_start + (sizeof(inodo) * lcarpeta.b_content[j].b_inodo);
                        fseek(archivo, posLectura, SEEK_SET);
                        fread(&tinodo, sizeof(inodo), 1, archivo);

                        //Verificar si es carpeta para revisar recursivamente
                        if(tinodo.i_type == '0'){
                            carpetas.push_back(carpeta);
                            buscar(sesion, sblock, ruta, lcarpeta.b_content[j].b_inodo, carpetas, nombre);
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
                                continue;
                            }

                            //Verificar si cumple el nombre
                            if(std::regex_match(carpeta, nombre)){
                                std::string imprimir = "";
                                for(int z = 0; z < carpetas.size(); z++){
                                    if(z == 0){
                                        imprimir.append("./");
                                    }else{
                                        imprimir.append("/");
                                    }
                                    imprimir.append(carpetas[z]);
                                }
                                imprimir.append("/");
                                imprimir.append(carpeta);
                                std::cout << imprimir << std::endl;
                            }

                            //Leer el inodo       
                            posLectura = sblock.s_inode_start + (sizeof(inodo) * lcarpeta.b_content[j].b_inodo);
                            fseek(archivo, posLectura, SEEK_SET);
                            fread(&tinodo, sizeof(inodo), 1, archivo);

                            //Verificar si es carpeta para revisar recursivamente
                            if(tinodo.i_type == '0'){
                                carpetas.push_back(carpeta);
                                buscar(sesion, sblock, ruta, lcarpeta.b_content[j].b_inodo, carpetas, nombre);
                            }
                        }
                    }
                }
            }
        }else if(i == 0){
            posLectura = sblock.s_block_start + (sizeof(bcarpetas) * linodo.i_block[i]);
            fseek(archivo, posLectura, SEEK_SET);
            fread(&lcarpeta, sizeof(bcarpetas), 1, archivo);

            for(int j = 2; j < 4; j++){
                std::string carpeta(lcarpeta.b_content[j].b_name);

                if(carpeta == "-"){
                    continue;
                }

                //Verificar si cumple el nombre
                if(std::regex_match(carpeta, nombre)){
                    std::string imprimir = "";
                    for(int z = 0; z < carpetas.size(); z++){
                        if(z == 0){
                            imprimir.append("./");
                        }else{
                            imprimir.append("/");
                        }
                        imprimir.append(carpetas[z]);
                    }
                    imprimir.append("/");
                    imprimir.append(carpeta);
                    std::cout << imprimir << std::endl;
                }

                //Leer el inodo       
                posLectura = sblock.s_inode_start + (sizeof(inodo) * lcarpeta.b_content[j].b_inodo);
                fseek(archivo, posLectura, SEEK_SET);
                fread(&tinodo, sizeof(inodo), 1, archivo);

                //Verificar si es carpeta para revisar recursivamente
                if(tinodo.i_type == '0'){
                    carpetas.push_back(carpeta);
                    buscar(sesion, sblock, ruta, lcarpeta.b_content[j].b_inodo, carpetas, nombre);
                }
            }
        }else{
            posLectura = sblock.s_block_start + (sizeof(bcarpetas) * linodo.i_block[i]);
            fseek(archivo, posLectura, SEEK_SET);
            fread(&lcarpeta, sizeof(bcarpetas), 1, archivo);

            for(int j = 0; j < 4; j++){
                std::string carpeta(lcarpeta.b_content[j].b_name);

                if(carpeta == "-"){
                    continue;
                }

                //Verificar si cumple el nombre
                if(std::regex_match(carpeta, nombre)){
                    std::string imprimir = "";
                    for(int z = 0; z < carpetas.size(); z++){
                        if(z == 0){
                            imprimir.append("./");
                        }else{
                            imprimir.append("/");
                        }
                        imprimir.append(carpetas[z]);
                    }
                    imprimir.append("/");
                    imprimir.append(carpeta);
                    std::cout << imprimir << std::endl;
                }

                //Leer el inodo       
                posLectura = sblock.s_inode_start + (sizeof(inodo) * lcarpeta.b_content[j].b_inodo);
                fseek(archivo, posLectura, SEEK_SET);
                fread(&tinodo, sizeof(inodo), 1, archivo);

                //Verificar si es carpeta para revisar recursivamente
                if(tinodo.i_type == '0'){
                    carpetas.push_back(carpeta);
                    buscar(sesion, sblock, ruta, lcarpeta.b_content[j].b_inodo, carpetas, nombre);
                }
            }
        }
    }
    
    carpetas.pop_back();
    fclose(archivo);
}
