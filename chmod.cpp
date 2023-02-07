#include "chmod.h"

void chmod(std::vector<std::string> &parametros, std::vector<disco> &discos, usuario &sesion){
    
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
    std::string ruta = "";                     //Atributo path
    std::string ugo = "";                      //Atributo ugo
    bool recursivo = false;                    //Atributo r
    std::string diskName = "";                 //Nombre del disco
    int posDisco = -1;                         //Posicion del disco dentro del vector
    int posParticion = -1;                     //Posicion de la particion dentro del vector del disco
    int posInicio;                             //Posicion donde inicia la particion
    int posLectura;                            //Para determinar la posicion de lectura en disco
    int inodo_buscado = -1;                    //Numero de Inodo del archivo o carpeta
    sbloque sblock;                            //Para leer el superbloque
    inodo linodo;                              //Para el manejo de los inodos
    bcarpetas lcarpeta;                        //Para el manejo de bloques de carpetas
    barchivos larchivo;                        //Para el manejo de bloques de archivo
    bapuntadores lapuntador;                   //Para el manejo de bloques de apuntadores simples
    bapuntadores lapuntador_doble;             //Para el manejo de bloques de apuntadores dobles
    bapuntadores lapuntador_triple;            //Para el manejo de bloques de apuntadores triples
    bool continuar = true;                     //Usado como bandera al buscar la ruta

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
                recursivo = true;
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
            }else if(tag == "ugo"){
                ugo = value;
            }else if(tag == "r"){
                std::cout << "ERROR: El parametro 'r' no recibe ningún valor." << std::endl;
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
    if(ruta == "" || ugo == ""){
        required = false;
    }

    if(!required){
        std::cout << "ERROR: La instrucción chmod carece de todos los parametros obligatorios." << std::endl;
    }

    //VALIDACION DE PARAMETROS
    if(ugo.size() != 3){
        std::cout << "ERROR: Los permisos están formados por 3 digitos entre 0 y 7." << std::endl;
        valid = false;
    }else{
        if((ugo[0] - '0') > 7 || (ugo[0] - '0') < 0){
            std::cout << "ERROR: El primer digito de los permisos debe estar entre 0 y 7." << std::endl;
            valid = false;
        }

        if((ugo[1] - '0') > 7 || (ugo[1] - '0') < 0){
            std::cout << "ERROR: El segundo digito de los permisos debe estar entre 0 y 7." << std::endl;
            valid = false;
        }

        if((ugo[2] - '0') > 7 || (ugo[2] - '0') < 0){
            std::cout << "ERROR: El tercer digito de los permisos debe estar entre 0 y 7." << std::endl;
            valid = false;
        }
    }

    if(!valid){
        return;
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
    fseek(archivo, posLectura, SEEK_SET);
    fread(&linodo, sizeof(inodo), 1, archivo);

    //BUSCAR EL INODO DE LA CARPETA O ARCHIVO A MODIFICAR
    posicion = 1;
    if(ruta == "/"){
        inodo_buscado = 0;
        continuar = false;
    }else if(path[0] != "\0"){
        continuar = false;
    }
    
    while(continuar){
        int inodo_temporal = -1;
        
        for(int i = 0; i < 15; i++){
            if(inodo_buscado != -1 || inodo_temporal != -1){
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
                                inodo_buscado = lcarpeta.b_content[k].b_inodo;
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
                                    inodo_buscado = lcarpeta.b_content[l].b_inodo;
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
                                        inodo_buscado = lcarpeta.b_content[m].b_inodo;
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
                            inodo_buscado = lcarpeta.b_content[j].b_inodo;
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

        if(inodo_buscado == -1){
            std::cout << "ERROR: La ruta ingresada es erronea." << std::endl;
            fclose(archivo);
            return;
        }
    }
    

    if(inodo_buscado == -1){
        std::cout << "ERROR: La ruta ingresada es erronea." << std::endl;
        fclose(archivo);
        return;
    }

    //LEER EL INODO BUSCADO 
    posLectura = sblock.s_inode_start + (sizeof(inodo) * inodo_buscado);
    fseek(archivo, posLectura, SEEK_SET);
    fread(&linodo, sizeof(inodo), 1, archivo);

    //MODIFICAR PERMISOS DEL INODO Y REESCRIBIRLO
    if(!recursivo || linodo.i_type != '0'){
        linodo.i_perm = std::stoi(ugo);
        linodo.i_mtime = time(NULL);
        fseek(archivo, posLectura, SEEK_SET);
        fwrite(&linodo, sizeof(inodo), 1, archivo);
    }
    
    //CAMBIAR PERMISOS RECURSIVAMENTE (DE SER EL CASO)
    bool cambiar = true;
    std::stack<int> inodos;
    if(recursivo && linodo.i_type == '0'){

        while(cambiar){
            //Cambiar permisos
            linodo.i_perm = std::stoi(ugo);
            linodo.i_mtime = time(NULL);
            fseek(archivo, posLectura, SEEK_SET);
            fwrite(&linodo, sizeof(inodo), 1, archivo);

            //Recorrer el inodo si es de carpeta y añadir posiciones a la pila
            if(linodo.i_type == '0'){
                for(int i = 0; i < 15; i++){
                    
                    if(linodo.i_block[i] == -1){
                        continue;
                    }
                    if(i == 0){
                        posLectura = sblock.s_block_start + (sizeof(bcarpetas) * linodo.i_block[i]);
                        fseek(archivo, posLectura, SEEK_SET);
                        fread(&lcarpeta, sizeof(bcarpetas), 1, archivo);

                        for(int j = 2; j < 4; j++){
                            if(lcarpeta.b_content[j].b_inodo != -1){
                                inodos.push(lcarpeta.b_content[j].b_inodo);
                            }
                        }

                    }else if(i == 12){
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
                                if(lcarpeta.b_content[k].b_inodo != -1){
                                    inodos.push(lcarpeta.b_content[k].b_inodo);
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
                                    if(lcarpeta.b_content[l].b_inodo != -1){
                                        inodos.push(lcarpeta.b_content[l].b_inodo);
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
                                        if(lcarpeta.b_content[m].b_inodo != -1){
                                            inodos.push(lcarpeta.b_content[m].b_inodo);
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
                            if(lcarpeta.b_content[j].b_inodo != -1){
                                inodos.push(lcarpeta.b_content[j].b_inodo);
                            }
                        }
                    }
                }
            }

            //Si la pila no está vacía, seleccionar el siguiente inodo
            if(inodos.empty()){
                cambiar = false;
            }else{
                inodo_buscado = inodos.top();
                inodos.pop();

                posLectura = sblock.s_inode_start + (sizeof(inodo) * inodo_buscado);
                fseek(archivo, posLectura, SEEK_SET);
                fread(&linodo, sizeof(inodo), 1, archivo);
            }
        }
        
    }

    //ESCRIBIR EN EL JOURNAL EL COMANDO
    if(sblock.s_filesystem_type == 3){
        registro creacion;
        int posRegistro = -1;

        //Buscar un Journal Vacio
        //En contenido: -r(T o F), ugo
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
            std::string contenido = "";
            if(recursivo){
                contenido.append("T");
            }else{
                contenido.append("F");
            }
            contenido.append(",");
            contenido.append(ugo);
            strcpy(creacion.comando ,"chmod");
            strcpy(creacion.path ,ruta.c_str());
            strcpy(creacion.nombre ,"");
            strcpy(creacion.contenido, contenido.c_str());
            creacion.fecha = time(NULL);
            fseek(archivo, posRegistro, SEEK_SET);
            fwrite(&creacion, sizeof(registro), 1, archivo);
        }
    }

    std::cout << "MENSAJE: Cambio de permisos efectuados correctamente." << std::endl;
    fclose(archivo);
}