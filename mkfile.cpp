#include "mkfile.h"

void mkfile(std::vector<std::string> &parametros, std::vector<disco> &discos, usuario &sesion){
    
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
    bool padre = false;                        //Atributo -r
    int tamaño = 0;                           //Atributo -s
    std::string ruta_contenido = "";           //Atributo -cont
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
    std::string ruta_copia;                    //Usada para el journal 
    
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

            if(tag == "r"){
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
                ruta_copia = value;
            }else if(tag == "s"){
                tamaño = std::stoi(value);
            }else if(tag == "cont"){
                ruta_contenido = value;
            }else if(tag == "r"){
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
        std::cout << "ERROR: La instrucción mkfile carece de todos los parametros obligatorios." << std::endl;
    }

    //VALIDACION DE PARAMETROS
    if(tamaño < 0){
        std::cout << "ERROR: El tamaño no debe de ser negativo." << std::endl;
        valid = false;
    }

    if(!valid){
        return;
    }

    //PREPARACIÓN DE PARAMETROS - Extraer de la ruta el nombre del archivo y eliminarlo
    nombre_archivo = ruta.substr(ruta.find_last_of("\\/") + 1, ruta.size() - 1);
    ruta = ruta.substr(0, ruta.find_last_of("\\/"));

    if(ruta.size() == 0){
        ruta = "/";
    }

    //PREPARACIÓN DE PARAMETROS - Crear las carpetas que no existen usando el comando mkdir
    if(padre){
        std::string comandos = "path->";
        comandos.append(ruta);
        std::vector<std::string> parametros_mkdir;
        parametros_mkdir.push_back("mkdir");
        parametros_mkdir.push_back("p");
        parametros_mkdir.push_back(comandos);

        mkdir(parametros_mkdir, discos, sesion);
    }

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
    inodo_leido = 0;
    fseek(archivo, posLectura, SEEK_SET);
    fread(&linodo, sizeof(inodo), 1, archivo);

    //BUSCAR SI EL ARCHIVO EN CONT EXISTE EN CASO SE USE Y LEERLO
    if(ruta_contenido != ""){
        bool continuar = true;

        //Separar los nombres que vengan en la ruta.
        std::vector<std::string> path_cont(std::sregex_token_iterator(ruta_contenido.begin(), ruta_contenido.end(), separador, -1),
                        std::sregex_token_iterator());

        posicion = 1;
        if(ruta_contenido == "/"){
            continuar = false;
        }else if(path_cont[0] != "\0"){
            continuar = false;
        }


        //Mover la posicion al inodo donde se encuentra el archivo
        while(continuar){
            int inodo_temporal = -1;
         
            //Buscar si existe la carpeta
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

                            if(carpeta == path_cont[posicion]){

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

                                if(carpeta == path_cont[posicion]){
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

                                    if(carpeta == path_cont[posicion]){
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

                        if(carpeta == path_cont[posicion]){
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

            if(inodo_temporal == -1){
                continuar = false;
                std::cout << "ERROR: La ruta ingresada del contenido no existe." << std::endl;
                inodo_leido = -1;
            }else if(posicion == path_cont.size() && linodo.i_type == '1'){
                continuar = false;
            }else if(posicion == path_cont.size() && linodo.i_type == '0'){
                continuar = false;
                std::cout << "ERROR: No se encontró el archivo con el contenido a copiar." << std::endl;
                inodo_leido = -1;
            }
        }

        if(inodo_leido == -1){
            fclose(archivo);
            return;
        }

        //Leer el contenido del archivo
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
                    contenido += temp;
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
                        contenido += temp;
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
                            contenido += temp;
                        }
                    }
                }
            }else{
                posLectura = sblock.s_block_start + (sizeof(barchivos) * linodo.i_block[i]);
                fseek(archivo, posLectura, SEEK_SET);
                fread(&larchivo, sizeof(barchivos), 1, archivo);

                temp = larchivo.b_content; 
                contenido += temp;
            }
        }
    }

    //DETERMINAR EL CONTENIDO DEL ARCHIVO EN CASO NO USAR CONT
    if(ruta_contenido == "" && tamaño != 0){
        int restante = tamaño;
        bool continuar = true;

        while(continuar){
            if(restante <= 10){
                for(int i = 0; i < restante; i++){
                    contenido.append(std::to_string(i));
                }
                continuar = false;
            }else{
                contenido.append("0123456789");
                restante -= 10;
            }
        }
    }

    //DETERMINAR EL NUMERO DE BLOQUES QUE VA A USAR EL ARCHIVO
    if(contenido.size() % 63 == 0){
        bloques = contenido.size() / 63;    
    }else{
        bloques = (contenido.size() / 63) + 1;
    }
    
    //AÑADIR LOS BLOQUES DE APUNTADORES AL CONTADOR
    int bloques_extras = bloques;
    bloques_extras -= 12; 
    if(bloques_extras > 0){
        bloques_extras -= 16;
        bloques += 1;
    }

    if(bloques_extras > 0){
        bloques += 1;
        for(int i = 0; i < 16; i++){
            bloques_extras -= 16;
            bloques += 1;

            if(bloques_extras <= 0){
                break;
            }
        }
    }

    if(bloques_extras > 0){
        bloques += 1;
        for(int i = 0; i < 16; i++){
            bloques += 1;
            for(int j = 0; j < 16; j++){
                bloques_extras -= 16;
                bloques += 1;

                if(bloques_extras <= 0){
                    break;
                }
            }
            
            if(bloques_extras <= 0){
                break;
            }
        }
    }

    if(bloques_extras > 0){
        std::cout << "ERROR: El archivo supera el limite permitido. No hay espacio en el inodo." << std::endl;
        fclose(archivo);
        return;
    }

    //DETERMINAR SI HAY ESPACIO PARA EL NUEVO ARCHIVO
    int espacios_vacios = 0;
    char a;
    for(int i = 0; i < sblock.s_blocks_count; i++){
        posLectura = sblock.s_bm_block_start + (sizeof(char) * i);
        fseek(archivo, posLectura, SEEK_SET);
        fread(&a, sizeof(char), 1, archivo);

        if(a == 'p' || a == 'a' || a == 'c'){
            
        }else{
            espacios_vacios += 1;
        }
    }

    if(espacios_vacios < bloques){
        std::cout << "ERROR: No hay bloques disponibles para escribir el archivo." << std::endl;
        fclose(archivo);
        return;
    }

    //BUSCAR LA CARPETA DONDE SE ALMACENARÁ EL ARCHIVO
    continuar = true;
    posLectura = sblock.s_inode_start;
    inodo_leido = 0;
    fseek(archivo, posLectura, SEEK_SET);
    fread(&linodo, sizeof(inodo), 1, archivo);

    posicion = 1;
    if(ruta == "/"){
        continuar = false;
    }else if(path[0] != "\0"){
        continuar = false;
    }

    while(continuar){
        int inodo_temporal = -1;
        
        //Buscar si existe la carpeta
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

        if(inodo_temporal == -1){
            continuar = false;
            std::cout << "ERROR: La ruta ingresada no existe." << std::endl;
            inodo_leido = -1;
        }else if(posicion == path.size() && linodo.i_type == '0'){
            continuar = false;
        }else if(posicion == path.size() && linodo.i_type == '1'){
            continuar = false;
            std::cout << "ERROR: No se encontró la carpeta para crear el archivo." << std::endl;
            inodo_leido = -1;
        }
    }

    if(inodo_leido == -1){
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
        std::cout << "ERROR: No posee permisos para escribir en esta carpeta." << std::endl;
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

                    strcpy(lcarpeta.b_content[j].b_name, nombre_archivo.c_str());
                    lcarpeta.b_content[j].b_inodo = inodo_temporal;
                    posLectura = sblock.s_block_start + (sizeof(bcarpetas) * linodo.i_block[i]);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&lcarpeta, sizeof(bcarpetas), 1, archivo);

                    //Actualizar el superbloque
                    sblock.s_free_inodes_count -= 1;
                    break;
                }else if(carpeta == nombre_archivo){
                    buscar = false;
                    //CONFIRMAR BORRADO
                    std::string confirmar;
                    std::cout << "¿Desea sobrescribir? [Y/N]" << std::endl;
                    std::getline(std::cin, confirmar);

                    if(confirmar == "N" || confirmar == "n"){
                        fclose(archivo);
                        return;
                    }else if(confirmar == "Y" || confirmar == "y"){
                        
                    }else{
                        std::cout <<"ERROR: Entrada inesperada." << std::endl;
                        fclose(archivo);
                        return;
                    }
                    inodo_temporal = lcarpeta.b_content[j].b_inodo;
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

            //Escribir el nuevo bloque de carpeta
            strcpy(ccarpeta.b_content[0].b_name, nombre_archivo.c_str());
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
            sblock.s_free_blocks_count -= 1;
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

            //Escribir el nuevo bloque de carpeta
            strcpy(ccarpeta.b_content[0].b_name, nombre_archivo.c_str());
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
            sblock.s_free_blocks_count -= 2;
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

                    //Escribir el nuevo bloque de carpeta
                    strcpy(ccarpeta.b_content[0].b_name, nombre_archivo.c_str());
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
                    sblock.s_free_blocks_count -= 1;
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

                            strcpy(lcarpeta.b_content[j].b_name, nombre_archivo.c_str());
                            lcarpeta.b_content[j].b_inodo = inodo_temporal;
                            posLectura = sblock.s_block_start + (sizeof(bcarpetas) * lapuntador.b_pointers[i]);
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&lcarpeta, sizeof(bcarpetas), 1, archivo);

                            //Actualizar el superbloque
                            sblock.s_free_inodes_count -= 1;
                            break;
                        }else if(carpeta == nombre_archivo){
                            buscar = false;
                        
                            std::string confirmar;
                            std::cout << "¿Desea sobrescribir? [Y/N]" << std::endl;
                            std::getline(std::cin, confirmar);

                            if(confirmar == "N" || confirmar == "n"){
                                fclose(archivo);
                                return;
                            }else if(confirmar == "Y" || confirmar == "y"){
                                
                            }else{
                                std::cout <<"ERROR: Entrada inesperada." << std::endl;
                                fclose(archivo);
                                return;
                            }
                            inodo_temporal = lcarpeta.b_content[j].b_inodo;
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

            //Escribir el nuevo bloque de carpeta
            strcpy(ccarpeta.b_content[0].b_name, nombre_archivo.c_str());
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
            sblock.s_free_blocks_count -= 3;
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

                    //Escribir el nuevo bloque de carpeta
                    strcpy(ccarpeta.b_content[0].b_name, nombre_archivo.c_str());
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
                    sblock.s_free_blocks_count -= 2;
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

                            //Escribir el nuevo bloque de carpeta
                            strcpy(ccarpeta.b_content[0].b_name, nombre_archivo.c_str());
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
                            sblock.s_free_blocks_count -= 1;
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

                                    strcpy(lcarpeta.b_content[k].b_name, nombre_archivo.c_str());
                                    lcarpeta.b_content[k].b_inodo = inodo_temporal;
                                    posLectura = sblock.s_block_start + (sizeof(bcarpetas) * linodo.i_block[j]);
                                    fseek(archivo, posLectura, SEEK_SET);
                                    fwrite(&lcarpeta, sizeof(bcarpetas), 1, archivo);

                                    //Actualizar el superbloque
                                    sblock.s_free_inodes_count -= 1;
                                    break;
                                }else if(carpeta == nombre_archivo){
                                    buscar = false;
                                
                                    std::string confirmar;
                                    std::cout << "¿Desea sobrescribir? [Y/N]" << std::endl;
                                    std::getline(std::cin, confirmar);

                                    if(confirmar == "N" || confirmar == "n"){
                                        fclose(archivo);
                                        return;
                                    }else if(confirmar == "Y" || confirmar == "y"){
                                        
                                    }else{
                                        std::cout <<"ERROR: Entrada inesperada." << std::endl;
                                        fclose(archivo);
                                        return;
                                    }
                                    inodo_temporal = lcarpeta.b_content[k].b_inodo;
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

            //Escribir el nuevo bloque de carpeta
            strcpy(ccarpeta.b_content[0].b_name, nombre_archivo.c_str());
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

                    //Escribir el nuevo bloque de carpeta
                    strcpy(ccarpeta.b_content[0].b_name, nombre_archivo.c_str());
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

                    sblock.s_free_blocks_count -= 3;
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

                            //Escribir el nuevo bloque de carpeta
                            strcpy(ccarpeta.b_content[0].b_name, nombre_archivo.c_str());
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

                            sblock.s_free_blocks_count -= 2;
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

                                    //Escribir el nuevo bloque de carpeta
                                    strcpy(ccarpeta.b_content[0].b_name, nombre_archivo.c_str());
                                    ccarpeta.b_content[0].b_inodo = inodo_temporal;
                                    posLectura = sblock.s_block_start + (sizeof(bcarpetas) * bloque_intermedio);
                                    fseek(archivo, posLectura, SEEK_SET);
                                    fwrite(&ccarpeta, sizeof(bcarpetas), 1, archivo);

                                    //Actualizar bloque de apuntadores
                                    lapuntador.b_pointers[k] = bloque_intermedio;
                                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * lapuntador_doble.b_pointers[j]);
                                    fseek(archivo, posLectura, SEEK_SET);
                                    fwrite(&lapuntador, sizeof(bapuntadores), 1, archivo);

                                    sblock.s_free_blocks_count -= 1;
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

                                            strcpy(lcarpeta.b_content[l].b_name, nombre_archivo.c_str());
                                            lcarpeta.b_content[l].b_inodo = inodo_temporal;
                                            posLectura = sblock.s_block_start + (sizeof(bcarpetas) * linodo.i_block[k]);
                                            fseek(archivo, posLectura, SEEK_SET);
                                            fwrite(&lcarpeta, sizeof(bcarpetas), 1, archivo);
                                            sblock.s_free_inodes_count -= 1;
                                            break;
                                        }else if(carpeta == nombre_archivo){
                                            buscar = false;
                                        
                                            std::string confirmar;
                                            std::cout << "¿Desea sobrescribir? [Y/N]" << std::endl;
                                            std::getline(std::cin, confirmar);

                                            if(confirmar == "N" || confirmar == "n"){
                                                fclose(archivo);
                                                return;
                                            }else if(confirmar == "Y" || confirmar == "y"){
                                                
                                            }else{
                                                std::cout <<"ERROR: Entrada inesperada." << std::endl;
                                                fclose(archivo);
                                                return;
                                            }
                                            inodo_temporal = lcarpeta.b_content[l].b_inodo;
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

    //Crear el inodo de carpeta
    cinodo.i_uid = std::stoi(sesion.id_user);
    cinodo.i_gid = std::stoi(sesion.id_grp);
    cinodo.i_s = contenido.size();
    cinodo.i_atime = time(NULL);
    cinodo.i_ctime = time(NULL);
    cinodo.i_mtime = time(NULL);
    for(int i = 0; i < 15; i++){
        cinodo.i_block[i] = -1;
    }
    cinodo.i_type = '1';
    cinodo.i_perm = 664;
    posLectura = sblock.s_inode_start + (sizeof(inodo) * inodo_temporal);
    fseek(archivo,posLectura , SEEK_SET);
    fwrite(&cinodo, sizeof(inodo), 1, archivo);

    //Actualizar Variables
    posLectura = sblock.s_inode_start + (sizeof(inodo) * inodo_temporal);
    fseek(archivo, posLectura, SEEK_SET);
    fread(&linodo, sizeof(inodo), 1, archivo);
    inodo_leido = inodo_temporal;

    //CREAR EL ARCHIVO
    std::string escribir = "";
    continuar = true;
    posicion = 0;

    while(continuar){
        bool revisar = true;
        int bloque_usado = -1;
        int bloque_apuntadors = -1;
        int bloque_apuntadord = -1;
        int bloque_apuntadort = -1;
        barchivos earchivo;
        bapuntadores eapuntador;
        bapuntadores eapuntador_doble;
        bapuntadores eapuntador_triple;

        for(int z = 0; z < 16; z++){
            eapuntador.b_pointers[z] = -1;
            eapuntador_doble.b_pointers[z] = -1;
            eapuntador_triple.b_pointers[z] = -1;
        }

        if(contenido.size() > 63){
            escribir = contenido.substr(0, 63); 
            contenido = contenido.substr(63, contenido.length()-1);
        }else{
            escribir = contenido;
            continuar = false;
        }
        
        while(revisar){

            if(posicion == 12){
                if(linodo.i_block[posicion] == -1){
                    //Escribir en el bitmap el bloque de apuntadores
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

                    //Escribir en el bitmap el bloque de archivos
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
                            return;
                        }
                    }

                    //Crear y escribir el bloque de archivos
                    strcpy(earchivo.b_content, escribir.c_str());
                    posLectura = sblock.s_block_start + (sizeof(barchivos) * bloque_usado);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&earchivo, sizeof(barchivos), 1, archivo);

                    //Crear y escribir el bloque de apuntadores
                    eapuntador.b_pointers[0] = bloque_usado;
                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * bloque_apuntadors);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&eapuntador, sizeof(bapuntadores), 1, archivo);

                    //Actualizar el inodo
                    linodo.i_block[posicion] = bloque_apuntadors;
                    sblock.s_free_blocks_count -= 2;
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
                                return;
                            }
                        }

                        //Crear y escribir el bloque
                        strcpy(earchivo.b_content, escribir.c_str());
                        posLectura = sblock.s_block_start + (sizeof(barchivos) * bloque_usado);
                        fseek(archivo, posLectura, SEEK_SET);
                        fwrite(&earchivo, sizeof(barchivos), 1, archivo);

                        //Actualizar y escribir el bloque de apuntadores
                        lapuntador.b_pointers[espacio] = bloque_usado;
                        posLectura = sblock.s_block_start + (sizeof(bapuntadores) * linodo.i_block[posicion]);;
                        fseek(archivo, posLectura, SEEK_SET);
                        fwrite(&lapuntador, sizeof(bapuntadores), 1, archivo);

                        //Actualizar variables
                        revisar = false;
                        sblock.s_free_blocks_count -= 1;
                    }
                }

            }else if(posicion == 13){
                if(linodo.i_block[posicion] == -1){
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
                            return;
                        }
                    }

                    //Escribir en el bitmap el bloque de apuntadores
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

                    //Escribir en el bitmap el bloque de archivos
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
                            return;
                        }
                    }

                    //Crear y escribir el bloque de archivos
                    strcpy(earchivo.b_content, escribir.c_str());
                    posLectura = sblock.s_block_start + (sizeof(barchivos) * bloque_usado);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&earchivo, sizeof(barchivos), 1, archivo);

                    //Crear y escribir el bloque de apuntadores 
                    eapuntador.b_pointers[0] = bloque_usado;
                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * bloque_apuntadors);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&eapuntador, sizeof(bapuntadores), 1, archivo);

                    //Crear y escribir el bloque de apuntadores dobles
                    eapuntador_doble.b_pointers[0] = bloque_apuntadors;
                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * bloque_apuntadord);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&eapuntador_doble, sizeof(bapuntadores), 1, archivo);

                    //Actualizar el inodo
                    linodo.i_block[posicion] = bloque_apuntadord;
                    sblock.s_free_blocks_count -= 3;
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

                            //Escribir en el bitmap el bloque de archivos
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
                                    return;
                                }
                            }

                            //Crear y escribir el bloque de archivos
                            strcpy(earchivo.b_content, escribir.c_str());
                            posLectura = sblock.s_block_start + (sizeof(barchivos) * bloque_usado);
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&earchivo, sizeof(barchivos), 1, archivo);

                            //Crear y escribir el bloque de apuntadores
                            eapuntador.b_pointers[0] = bloque_usado;
                            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * bloque_apuntadors);
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&eapuntador, sizeof(bapuntadores), 1, archivo);

                            //Actualizar el apuntador
                            lapuntador_doble.b_pointers[i] = bloque_apuntadors;
                            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * linodo.i_block[posicion]);;
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&lapuntador_doble, sizeof(bapuntadores), 1, archivo);

                            //Actualizar Variables
                            revisar = false;
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
                                        return;
                                    }
                                }

                                //Crear y escribir el bloque
                                strcpy(earchivo.b_content, escribir.c_str());
                                posLectura = sblock.s_block_start + (sizeof(barchivos) * bloque_usado);
                                fseek(archivo, posLectura, SEEK_SET);
                                fwrite(&earchivo, sizeof(barchivos), 1, archivo);

                                //Actualizar y escribir el bloque de apuntadores
                                lapuntador.b_pointers[espacio] = bloque_usado;
                                posLectura = sblock.s_block_start + (sizeof(bapuntadores) * lapuntador_doble.b_pointers[i]);;
                                fseek(archivo, posLectura, SEEK_SET);
                                fwrite(&lapuntador, sizeof(bapuntadores), 1, archivo);

                                //Actualizar variables
                                revisar = false;
                                sblock.s_free_blocks_count -= 1;
                                break;
                            }
                        }
                    }
                }
            }else if(posicion == 14){
                if(linodo.i_block[posicion] == -1){
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
                            return;
                        }
                    }

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
                            return;
                        }
                    }

                    //Escribir en el bitmap el bloque de apuntadores
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

                    //Escribir en el bitmap el bloque de archivos
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
                            return;
                        }
                    }

                    //Crear y escribir el bloque de archivos
                    strcpy(earchivo.b_content, escribir.c_str());
                    posLectura = sblock.s_block_start + (sizeof(barchivos) * bloque_usado);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&earchivo, sizeof(barchivos), 1, archivo);

                    //Crear y escribir el bloque de apuntadores 
                    eapuntador.b_pointers[0] = bloque_usado;
                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * bloque_apuntadors);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&eapuntador, sizeof(bapuntadores), 1, archivo);

                    //Crear y escribir el bloque de apuntadores dobles
                    eapuntador_doble.b_pointers[0] = bloque_apuntadors;
                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * bloque_apuntadord);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&eapuntador_doble, sizeof(bapuntadores), 1, archivo);

                    //Crear y escribir el bloque de apuntadores triples
                    eapuntador_triple.b_pointers[0] = bloque_apuntadord;
                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * bloque_apuntadort);
                    fseek(archivo, posLectura, SEEK_SET);
                    fwrite(&eapuntador_triple, sizeof(bapuntadores), 1, archivo);

                    //Actualizar el inodo
                    linodo.i_block[posicion] = bloque_apuntadort;
                    revisar = false;
                    sblock.s_free_blocks_count -= 4;
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

                            //Escribir en el bitmap el bloque de apuntadores
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

                            //Escribir en el bitmap el bloque de archivos
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
                                    return;
                                }
                            }

                            //Crear y escribir el bloque de archivos
                            strcpy(earchivo.b_content, escribir.c_str());
                            posLectura = sblock.s_block_start + (sizeof(barchivos) * bloque_usado);
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&earchivo, sizeof(barchivos), 1, archivo);

                            //Crear y escribir el bloque de apuntadores
                            eapuntador.b_pointers[0] = bloque_usado;
                            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * bloque_apuntadors);
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&eapuntador, sizeof(bapuntadores), 1, archivo);

                            //Crear y escribir el bloque de apuntadores dobles
                            eapuntador_doble.b_pointers[0] = bloque_apuntadors;
                            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * bloque_apuntadord);
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&eapuntador_doble, sizeof(bapuntadores), 1, archivo);

                            //Actualizar el apuntador
                            lapuntador_triple.b_pointers[i] = bloque_apuntadord;
                            posLectura = sblock.s_block_start + (sizeof(bapuntadores) * linodo.i_block[posicion]);;
                            fseek(archivo, posLectura, SEEK_SET);
                            fwrite(&lapuntador_triple, sizeof(bapuntadores), 1, archivo);

                            //Actualizar Variables
                            revisar = false;
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
                                    //Escribir en el bitmap el bloque de apuntadores
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

                                    //Escribir en el bitmap el bloque de archivos
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
                                            return;
                                        }
                                    }

                                    //Crear y escribir el bloque de archivos
                                    strcpy(earchivo.b_content, escribir.c_str());
                                    posLectura = sblock.s_block_start + (sizeof(barchivos) * bloque_usado);
                                    fseek(archivo, posLectura, SEEK_SET);
                                    fwrite(&earchivo, sizeof(barchivos), 1, archivo);

                                    //Crear y escribir el bloque de apuntadores
                                    eapuntador.b_pointers[0] = bloque_usado;
                                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * bloque_apuntadors);
                                    fseek(archivo, posLectura, SEEK_SET);
                                    fwrite(&eapuntador, sizeof(bapuntadores), 1, archivo);

                                    //Actualizar el apuntador
                                    lapuntador_doble.b_pointers[j] = bloque_apuntadors;
                                    posLectura = sblock.s_block_start + (sizeof(bapuntadores) * lapuntador_triple.b_pointers[i]);;
                                    fseek(archivo, posLectura, SEEK_SET);
                                    fwrite(&lapuntador_doble, sizeof(bapuntadores), 1, archivo);

                                    //Actualizar Variables
                                    revisar = false;
                                    salir = true;
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
                                            espacio = k;
                                            break;
                                        }
                                    }

                                    if(espacio != -1){
                                        //Escribir en el bitmap el bloque
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
                                                return;
                                            }
                                        }

                                        //Crear y escribir el bloque
                                        strcpy(earchivo.b_content, escribir.c_str());
                                        posLectura = sblock.s_block_start + (sizeof(barchivos) * bloque_usado);
                                        fseek(archivo, posLectura, SEEK_SET);
                                        fwrite(&earchivo, sizeof(barchivos), 1, archivo);

                                        //Actualizar y escribir el bloque de apuntadores
                                        lapuntador.b_pointers[espacio] = bloque_usado;
                                        posLectura = sblock.s_block_start + (sizeof(bapuntadores) * lapuntador_doble.b_pointers[j]);;
                                        fseek(archivo, posLectura, SEEK_SET);
                                        fwrite(&lapuntador, sizeof(bapuntadores), 1, archivo);

                                        //Actualizar variables
                                        revisar = false;
                                        salir = true; 
                                        sblock.s_free_blocks_count -= 1;
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
                        return;
                    }
                }

                //Crear y escribir el bloque
                strcpy(earchivo.b_content, escribir.c_str());
                posLectura = sblock.s_block_start + (sizeof(barchivos) * bloque_usado);
                fseek(archivo, posLectura, SEEK_SET);
                fwrite(&earchivo, sizeof(barchivos), 1, archivo);

                //Actualizar el inodo
                linodo.i_block[posicion] = bloque_usado;
                posicion += 1;
                revisar = false;

                sblock.s_free_blocks_count -= 1;
            }
        }
    }
                    
    //ESCRIBIR EL INODO CON TODOS LOS CAMBIOS
    linodo.i_mtime = time(NULL);
    posLectura = sblock.s_inode_start + (sizeof(inodo) * inodo_leido);
    fseek(archivo, posLectura, SEEK_SET);
    fwrite(&linodo, sizeof(inodo), 1, archivo);

    //ACTUALIZAR EL SUPERBLOQUE
    fseek(archivo, posInicio, SEEK_SET);
    fwrite(&sblock, sizeof(sbloque), 1, archivo);
 
    //ESCRIBIR EN EL JOURNAL EL COMANDO
    //En contenido: -r, -s, -cont
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
            std::string cont = "";
            if(padre){
                cont.append("T");
            }else{
                cont.append("F");
            }
            cont.append(",");
            cont.append(std::to_string(tamaño));
            cont.append(",");
            cont.append(ruta_contenido);

            strcpy(creacion.comando ,"mkfile");
            strcpy(creacion.path ,ruta_copia.c_str());
            strcpy(creacion.nombre ,nombre_archivo.c_str());
            strcpy(creacion.contenido, cont.c_str());
            creacion.fecha = time(NULL);
            fseek(archivo, posRegistro, SEEK_SET);
            fwrite(&creacion, sizeof(registro), 1, archivo);
        }
    }

    std::cout << "MENSAJE: Archivo creado correctamente." << std::endl;
    fclose(archivo);

}