#include "cat.h"


void cat(std::vector<std::string> &parametros, std::vector<disco> &discos, usuario &sesion){
    
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
    std::vector<std::string> ruta;         //Atributo fileN
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
    int inodo_leido = -1;                      //Numero de inodo que se está leyendo
    std::string contenido = "";                //Texto a imprimir           
    std::regex separador("/");       

    //COMPROBACIÓN DE PARAMETROS
    ruta = {"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""};
    int contador = 0;
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

        //Extraer el número y el nombre
        std::string param = tag.substr(0 ,4);
        int n = -1;

        try{
            n = std::stoi(tag.substr(4 ,tag.size() - 1));
        }catch(...){
            std::cout << "ERROR: El parametro " << tag << " no es valido." << std::endl;
            paramFlag = false;
            break;
        }

        //Verificar que se llame file
        if(param != "file"){
            std::cout << "ERROR: El parametro " << tag << " no es valido." << std::endl;
            paramFlag = false;
            break;
        }

        //Verificar que tenga un número
        if(n <= 0){
            std::cout << "ERROR: El parametro " << tag << " no es valido." << std::endl;
            paramFlag = false;
            break;
        }

        //Añadir la ruta al vector de direcciones
        ruta.insert(ruta.begin() + n, value);
        contador += 1;
    }

    if(!paramFlag){
        return;
    }

    //COMPROBAR PARAMETROS OBLIGATORIOS
    if(contador == 0){
        required = false;
    }

    if(!required){
        std::cout << "ERROR: La instrucción cat carece de todos los parametros obligatorios." << std::endl;
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

    //RECORRER EL VECTOR DE ARCHIVOS
    for(int i = 0; i < ruta.size(); i++){
        if(ruta[i] == ""){
            continue;
        }

        //Leer el inodo raíz
        posLectura = sblock.s_inode_start;
        fseek(archivo, posLectura, SEEK_SET);
        fread(&linodo, sizeof(inodo), 1, archivo);

        //Separar la ruta
        std::vector<std::string> path(std::sregex_token_iterator(ruta[i].begin(), ruta[i].end(), separador, -1),
                    std::sregex_token_iterator());

        //Buscar la carpeta del archivo
        posicion = 1;
        continuar = true;
        inodo_leido = -1;
        if(ruta[i] == "/"){
            std::cout << "ERROR: Las rutas no deben de pertenecer a carpetas." << std::endl;
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
        
        //Leer el inodo
        posLectura = sblock.s_inode_start + (sizeof(inodo) * inodo_leido);
        fseek(archivo, posLectura, SEEK_SET);
        fread(&linodo, sizeof(inodo), 1, archivo);

        if(linodo.i_type == '0'){
            std::cout << "ERROR: El comando cat no trabaja con carpetas." << std::endl;
            fclose(archivo);
            return;
        }

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
            std::cout << "ERROR: No posee permisos para renombrar." << std::endl;
            fclose(archivo);
            return;
        }

        //Leer el archivo
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

        contenido += "\n";
    }

    //IMPRIMIR LOS ARCHIVOS
    std::cout << contenido << std::endl;

    fclose(archivo);
    std::cout << "MENSAJE: Archivos leídos correctamente." << std::endl;
}
