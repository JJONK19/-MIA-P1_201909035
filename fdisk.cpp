#include "fdisk.h"

void fdisk(std::vector<std::string> &parametros){
    //VARIABLES
    bool paramFlag = true;                     //Indica si se cumplen con los parametros del comando
    bool required = true;                      //Indica si vienen los parametros obligatorios
    bool valid = true;                         //Verifica que los valores de los parametros sean correctos
    FILE *archivo;                             //Sirve para verificar que el archivo exista
    char vacio = '\0';                         //Usado para escribir el archivo binario
    int tamaño = 0;                            //Atrubuto -s
    std::string fit = "";                      //Atributo -f
    char fit_char = '0';                       //El fit se maneja como caracter 
    std::string unidad = "";                   //Atributo -u        
    std::string ruta = "";                     //Atributo path
    std::string tipo = "";                     //Atributo -t
    char tipo_char = '0';                      //El tipo se maneja como char
    std::string borrar = "";                   //Atributo delete
    std::string nombre = "";                   //Atributo name
    int añadir = 0;                            //Atributo add      
    std::string comando = "";                  //Indica si se crea/borra/expande al ejecutar
 
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

        if(tag == "s"){
            tamaño = stoi(value);
            if(comando == ""){
                comando = "create";
            }
        }else if(tag == "u"){
            unidad = value;
        }else if(tag == "path"){
            ruta = value;
        }else if(tag == "t"){
            tipo = value;
        }else if(tag == "f"){
            fit = value;
        }else if(tag == "delete"){
            borrar = value;
            if(comando == ""){
                comando = "delete";
            }
        }else if(tag == "name"){
            nombre = value;
        }else if(tag == "add"){
            añadir = stoi(value);
            if(comando == ""){
                comando = "add";
            }
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
    if(nombre == "0" || ruta == ""){
            required = false;
    }

    if(!required){
        std::cout << "ERROR: La instrucción fdisk carece de todos los parametros obligatorios." << std::endl;
    }

    //VALIDACION DE PARAMETROS
    transform(fit.begin(), fit.end(), fit.begin(),[](unsigned char c){
        return tolower(c);
    });

    transform(tipo.begin(), tipo.end(), tipo.begin(),[](unsigned char c){
        return tolower(c);
    });

    transform(unidad.begin(), unidad.end(), unidad.begin(),[](unsigned char c){
        return tolower(c);
    });

    transform(borrar.begin(), borrar.end(), borrar.begin(),[](unsigned char c){
        return tolower(c);
    });

    if(comando == "create" && tamaño <= 0){
        std::cout << "ERROR: El tamaño de la nueva partición debe de ser mayor que 0." << std::endl;
        valid = false;
    }
    
    if(comando == "add" && añadir == 0){
        std::cout << "ERROR: El tamaño a modificar no puede ser 0." << std::endl;
        valid = false;
    }
    
    if(fit == "bestfit" || fit == "firstfit" || fit == "worstfit" || fit == ""){

    }else{
        std::cout << "ERROR: Tipo de Fit Invalido." << std::endl;
        valid = false;
    }

    if(tipo == "p" || tipo == "e" || tipo == "l" || tipo == ""){

    }else{
        std::cout << "ERROR: Tipo de Particion Invalido." << std::endl;
        valid = false;
    }

    if(unidad == "k" || unidad == "m" || unidad == "b"|| unidad == ""){

    }else{
        std::cout << "ERROR: Tipo de Unidad Invalido." << std::endl;
        valid = false;
    }

    if(borrar == "full" || borrar == ""){
    }else{
        std::cout << "ERROR: Utilice FULL junto al comando delete." << std::endl;
        valid = false;
    }

    if(comando == ""){
        std::cout << "ERROR: La instrucción carece de una tarea (añadir, borrar o modificar)." << std::endl;
        valid = false;
    }

    if(!valid){
        return;
    }

    //PREPARACIÓN DE PARAMETROS - Determinar el alias del fit y pasar a bytes el tamaño
    if(fit == "firstfit"){
        fit_char = 'f';
    }else if(fit == "bestfit"){
        fit_char = 'b';
    }else if(fit == "worstfit" || fit == "" ){
        fit_char = 'w';
    }

    if(tipo == "" || tipo == "p"){
        tipo_char = 'p';
    }else if(tipo == "e"){
        tipo_char = 'e';
    }else if(tipo == "l"){
        tipo_char = 'l';
    }

    if(unidad == "m"){
        tamaño = tamaño * 1024 * 1024;
    }else if(unidad == "" || unidad == "k"){
        tamaño = tamaño * 1024;
    }

    if(unidad == "m"){
        añadir = añadir * 1024 * 1024;
    }else if(unidad == "" || unidad == "k"){
        añadir = añadir * 1024;
    }

    //VERIFICAR QUE EL ARCHIVO EXISTA
    archivo = fopen(ruta.c_str(), "r+b");

    if(archivo == NULL){
        std::cout << "ERROR: El disco no existe." << std::endl;
        return;
    }else{
        fclose(archivo);
    }

    //SEPARAR TIPO DE OPERACIÓN Y EJECUTARLA
    if(comando == "create"){
        crear_particion(tamaño, tipo_char, ruta, nombre, fit_char);
    }else if(comando == "delete"){
        borrar_particion(ruta, nombre);
    }else{
        modificar_particion(tipo_char, ruta, añadir, nombre);
    }

}

void crear_particion(int &tamaño, char &tipo, std::string &ruta, std::string &nombre, char &fit){
    //VARIABLES GENERALES
    FILE *archivo = fopen(ruta.c_str(), "r+b");          //Disco con el que se va a trabajar
    MBR mbr;                                             //Para leer el mbr del disco  
    int posicion = -1;                                   //Posicion de la particion en el mbr               

    //COLOCAR EL PUNTERO DE LECTURA EN LA CABECERA Y LEER MBR
    fseek(archivo, 0, SEEK_SET);
    fread(&mbr, sizeof(MBR), 1, archivo);

    //=============== PARTICIONES LÓGICAS =============== 
    if(tipo == 'l'){
        //VARIABLES 
        int posExtendida;                                //Posicion de la cabecera de la extendida
        int finExtendida;                                //Posicion donde acaba la extendida
        EBR ebr;                                         //Auxiliar para leer los ebr
        std::vector<libreL> espacios;                    //Posiciones de los espacios libres
        bool cabecera_visitada = false;                  //Indica si es la cabecera la revisada
        bool continuar = true;                           //Salir del while de busqueda de espacios 
        bool existe = false;                             //Indica si existe la particion
        int contador = 0;                                //Numero de particiones en la extendida
        int posEspacio = -1;                             //Posicion del espacio a usar dentro del vector de espacios 
        libreL temp;                                     //Auxiliar para acceder a los datos del espacio vacio
        
        //BUSCAR LA PARTICIÓN EXTENDIDA
        for(int i = 0; i < 4; i++){
            if(mbr.mbr_partition[i].part_type == 'e'){
                posicion = i;
                break;
            }
        }

        if(posicion == -1){
            std::cout << "ERROR: No existe una partición extendida. Partición no creada." << std::endl; 
            fclose(archivo);
            return;
        }

        //MOVER EL PUNTERO A LA EXTENDIDA Y LEER EL EBR CABECERA
        posExtendida = mbr.mbr_partition[posicion].part_start;
        finExtendida = mbr.mbr_partition[posicion].part_start + mbr.mbr_partition[posicion].part_s; 
        fseek(archivo, posExtendida, SEEK_SET);
        fread(&ebr, sizeof(EBR), 1, archivo);

        //BUSCAR SI ESTA REPETIDA LA PARTICION Y ENCONTRAR ESPACIOS VACIOS
        while(continuar){
            if(ebr.part_name == nombre){
                existe = true;
                break;
            }
 
            if(!cabecera_visitada){
                contador++;
                if(ebr.part_s == 0){
                    libreL temp;
                    temp.cabecera = true;
                    temp.inicioEBR = posExtendida;
                    temp.finLogica = ebr.part_start;
                    if(ebr.part_next == -1){
                        temp.tamaño = finExtendida - temp.finLogica; 
                    }else{
                        temp.tamaño = ebr.part_next - temp.finLogica;
                    }

                    if(temp.tamaño > 0){
                        espacios.push_back(temp);
                    }
                }else{
                    libreL temp;
                    temp.cabecera = false;
                    temp.inicioEBR = posExtendida;
                    temp.finLogica = ebr.part_start + ebr.part_s - 1;
                    if(ebr.part_next == -1){
                        temp.tamaño = finExtendida - (temp.finLogica + 1); 
                    }else{
                        temp.tamaño = ebr.part_next - (temp.finLogica + 1);
                    }

                    if(temp.tamaño > 0){
                        espacios.push_back(temp);
                    }
                }

                cabecera_visitada = true;
                if(ebr.part_next == -1){
                    continuar = false;
                }else{
                    posExtendida = ebr.part_next;
                    fseek(archivo, posExtendida, SEEK_SET);
                    fread(&ebr, sizeof(EBR), 1, archivo);
                }
            }else{
                contador++;
                libreL temp;
                temp.cabecera = false;
                temp.inicioEBR = posExtendida;
                temp.finLogica = ebr.part_start + ebr.part_s - 1;
                if(ebr.part_next == -1){
                    temp.tamaño = finExtendida - (temp.finLogica + 1); 
                }else{
                    temp.tamaño = ebr.part_next - (temp.finLogica + 1);
                }

                if(temp.tamaño > 0){
                    espacios.push_back(temp);
                }
                if(ebr.part_next == -1){
                    continuar = false;
                }else{
                    posExtendida = ebr.part_next;
                    fseek(archivo, posExtendida, SEEK_SET);
                    fread(&ebr, sizeof(EBR), 1, archivo);
                }
            }
        }


        if(existe){
            std::cout << "ERROR: Ya existe una partición lógica con ese nombre." << std::endl; 
            fclose(archivo);
            return;
        }

        //VERIFICAR QUE NO SOBREPASE EL LIMITE
        if(contador >= 24){
            std::cout << "ERROR: Se ha alcanzado el máximo de particiones lógicas (24 particiones)." << std::endl;
            fclose(archivo);
            return;
        }

        //COMPROBAR QUE HAY ESPACIOS VACÍOS EN EL DISCO
        if(espacios.size() == 0){
            std::cout << "ERROR: La partición expandida se encuentra totalmnte ocupada." << std::endl;
            fclose(archivo);
            return;
        }

        //BUSCAR ESPACIO PARA INSERTAR - FIRST FIT
        if(fit == 'f'){                
            for(int i = 0; i < espacios.size(); i++){
                temp = espacios[i];
                if((tamaño + sizeof(EBR)) <= temp.tamaño){
                    posEspacio = i;
                    break;
                }
            }

            if(posEspacio == -1){
                std::cout << "ERROR: No hay espacio disponible en la partición extendida." << std::endl;
                fclose(archivo);
                return;
            }
        }

        //BUSCAR ESPACIO PARA INSERTAR - BEST FIT
        if(fit == 'b'){                
            sort(espacios.begin(), espacios.end());
            for(int i = 0; i < espacios.size(); i++){
                temp = espacios[i];
                if((tamaño + sizeof(EBR)) <= temp.tamaño){
                    posEspacio = i;
                    break;
                }
            }

            if(posEspacio == -1){
                std::cout << "ERROR: No hay espacio disponible en la partición extendida." << std::endl;
                fclose(archivo);
                return;
            }
        }

        //BUSCAR ESPACIO PARA INSERTAR - WORST FIT
        if(fit == 'w'){                
            sort(espacios.begin(), espacios.end());
            if(tamaño <= espacios[espacios.size() - 1].tamaño){
                posEspacio = espacios.size() - 1;
            }

            if(posEspacio == -1){
                std::cout << "ERROR: No hay espacio disponible en la partición extendida." << std::endl;
                fclose(archivo);
                return;
            }
        }

        //CREAR LA PARTICION
        if(espacios[posEspacio].cabecera){
            //Leer y reescribir EBR padre
            fseek(archivo, espacios[posEspacio].inicioEBR ,SEEK_SET);
            fread(&ebr, sizeof(EBR), 1, archivo);
            strcpy(ebr.part_name, nombre.c_str());
            ebr.part_fit = fit;
            ebr.part_s = tamaño;
            ebr.part_status = '0';
            fseek(archivo, espacios[posEspacio].inicioEBR ,SEEK_SET);
            fwrite(&ebr, sizeof(EBR), 1, archivo);

            fclose(archivo);
            std::cout << "MENSAJE: Particion lógica creada correctamente." << std::endl;
        }else{
            int posEBR = espacios[posEspacio].finLogica + 1;
            int next;
            //Leer y reescribir EBR padre
            fseek(archivo, espacios[posEspacio].inicioEBR ,SEEK_SET);
            fread(&ebr, sizeof(EBR), 1, archivo);
            next = ebr.part_next;
            ebr.part_next = posEBR;
            fseek(archivo, espacios[posEspacio].inicioEBR ,SEEK_SET);
            fwrite(&ebr, sizeof(EBR), 1, archivo);
            
            //Crear nueva particion (EBR)
            ebr.part_fit = fit;
            strcpy(ebr.part_name, nombre.c_str());
            ebr.part_status = '0';
            ebr.part_s = tamaño;
            ebr.part_start = posEBR + sizeof(EBR);   
            ebr.part_next = next;
            fseek(archivo, posEBR, SEEK_SET);
            fwrite(&ebr, sizeof(EBR), 1, archivo);
            
            fclose(archivo);
            std::cout << "MENSAJE: Particion lógica creada correctamente." << std::endl;
        }

    }

    //=============== PARTICIONES PRIMARIAS / EXPANDIDA ===============
    if(tipo == 'p' || tipo == 'e'){
        //VARIABLES 
        bool extendedExist = false;                 //Verifica que no exista una extendida
        bool existe = false;                        //Indica si existe la particion
        std::vector<position> posiciones;           //Posiciones de las particiones  
        std::vector<libre> espacios;                //Espacios vacios entre particiones   
        int posEspacio = -1;                        //Posicion en la lista de espacios libres
        libre temp;                                 //Auxiliar para leer espacios

        //BUSCAR SI HAY ESPACIO EN EL MBR Y DE PASO VER SI EXISTE LA EXTENDIDA
        for(int i = 0; i < 4; i++){
            if(mbr.mbr_partition[i].part_name[0] == '\0'){
                posicion = i;
            }
                
            if(mbr.mbr_partition[i].part_type == 'e'){
                extendedExist = true;
            }

            if(posicion != -1){
                break;
            }
        }

        if(extendedExist == true && tipo == 'e'){
            std::cout << "ERROR: Solo puede existir una partición extendida a la vez." << std::endl;
            fclose(archivo);
            return;
        }

        if(posicion == -1){
            std::cout << "ERROR: Limite de particiones alcanzado (4). Elimine una particion para continuar." << std::endl; 
            fclose(archivo);
            return;
        }

        //DETERMINAR SI EXISTE LA PARTICION Y MARCAR LAS POSICIONES DE CADA PARTICION
        for(int i = 0; i < 4; i++){
            if(mbr.mbr_partition[i].part_name[0] != '\0'){
                position temp;
                temp.inicio = mbr.mbr_partition[i].part_start;
                temp.fin = mbr.mbr_partition[i].part_start + mbr.mbr_partition[i].part_s - 1;
                temp.nombre = mbr.mbr_partition[i].part_name;
                posiciones.push_back(temp);

                if(nombre == mbr.mbr_partition[i].part_name){
                    existe = true;
                }
            }
        }

        if(existe){
            std::cout << "ERROR: Ya existe una particion con el nombre indicado." << std::endl;
            fclose(archivo);
            return;
        }

        //ORDENAR LAS PARTICIONES 
        if(posiciones.size() != 0){
            sort(posiciones.begin(), posiciones.end());
        }

        //CREAR LA LISTA DE ESPACIOS VACIOS
        if(posiciones.size() == 0){
            libre temp;
            temp.inicio = EndMBR;
            temp.tamaño = mbr.mbr_tamano - EndMBR;
            espacios.push_back(temp);
        }else{
            for(int i = 0; i < posiciones.size(); i++){
                libre temp;
                position &x = posiciones[i];
                int free = 0;
                if(i == 0 && i != (posiciones.size()-1)){
                    //Espacio entre el inicio y la primera particion
                    free = x.inicio - EndMBR;
                    if(free > 0){
                        temp.inicio = EndMBR;
                        temp.tamaño = free;
                        espacios.push_back(temp);
                    }

                    //Espacio entre la primera particion y la siguiente
                    position &y = posiciones[i + 1];
                    free = y.inicio - (x.fin + 1);
                    if(free > 0){
                        temp.inicio = x.fin + 1;
                        temp.tamaño = free;
                        espacios.push_back(temp);
                    }
                }else if(i == 0 && i == (posiciones.size()-1)){
                    //Espacio entre el inicio y la primera particion
                    free = x.inicio - EndMBR;
                    if(free > 0){
                        temp.inicio = EndMBR;
                        temp.tamaño = free;
                        espacios.push_back(temp);
                    }

                    //Espacio entre la primera particion y el fin
                    free = mbr.mbr_tamano - (x.fin + 1);
                    if(free > 0){
                        temp.inicio = x.fin + 1;
                        temp.tamaño = free;
                        espacios.push_back(temp);
                    }
                }else if(i != (posiciones.size()-1)){
                    position &y = posiciones[i + 1];
                    free = y.inicio - (x.fin + 1);
                    if(free > 0){
                        temp.inicio = x.fin + 1;
                        temp.tamaño = free;
                        espacios.push_back(temp);
                    }
                }else{
                    free = mbr.mbr_tamano - (x.fin + 1);
                    if(free > 0){
                        temp.inicio = x.fin + 1;
                        temp.tamaño = free;
                        espacios.push_back(temp);
                    }
                }
            }
        }

        if(espacios.size() == 0){
            std::cout << "ERROR: No hay espacio disponible en el disco." << std::endl;
            fclose(archivo);
            return;
        }

        //BUSCAR ESPACIO PARA INSERTAR - FIRST FIT
        if(fit == 'f'){                
            for(int i = 0; i < espacios.size(); i++){
                temp = espacios[i];
                if(tamaño <= temp.tamaño){
                    posEspacio = i;
                    break;
                }
            }

            if(posEspacio == -1){
                std::cout << "ERROR: No hay espacio disponible en el disco." << std::endl;
                fclose(archivo);
                return;
            }
        }

        //BUSCAR ESPACIO PARA INSERTAR - BEST FIT
        if(fit == 'b'){                
            sort(espacios.begin(), espacios.end());
            for(int i = 0; i < espacios.size(); i++){
                temp = espacios[i];
                if(tamaño <= temp.tamaño){
                    posEspacio = i;
                    break;
                }
            }

            if(posEspacio == -1){
                std::cout << "ERROR: No hay espacio disponible en el disco." << std::endl;
                fclose(archivo);
                return;
            }
        }

        //BUSCAR ESPACIO PARA INSERTAR - WORST FIT
        if(fit == 'w'){                
            sort(espacios.begin(), espacios.end());
            if(tamaño <= espacios[espacios.size() - 1].tamaño){
                posEspacio = espacios.size() - 1;
            }

            if(posEspacio == -1){
                std::cout << "ERROR: No hay espacio disponible en el disco." << std::endl;
                fclose(archivo);
                return;
            }
        }

        //CREAR PARTICION
        mbr.mbr_partition[posicion].part_fit = fit;
        strcpy(mbr.mbr_partition[posicion].part_name, nombre.c_str());
        mbr.mbr_partition[posicion].part_status = '0';
        mbr.mbr_partition[posicion].part_s = tamaño;
        mbr.mbr_partition[posicion].part_type = tipo;
        mbr.mbr_partition[posicion].part_start = espacios[posEspacio].inicio;   

        fseek(archivo, 0, SEEK_SET);                  
        fwrite(&mbr, sizeof(MBR), 1, archivo);

        //CREAR EL EBR INICIAL EN CASO DE SER EXTENDIDA
        if(tipo = 'e'){
            EBR ebr;
            strcpy(ebr.part_name, "");
            ebr.part_next = -1;
            ebr.part_start = espacios[posEspacio].inicio + sizeof(EBR) + 1;
            ebr.part_s = 0;
            ebr.part_status = '0';
            fseek(archivo, espacios[posEspacio].inicio ,SEEK_SET);
            fwrite(&ebr, sizeof(EBR), 1, archivo);
        }
        
        fclose(archivo);
        std::cout << "MENSAJE: Particion creada correctamente." << std::endl;
    }                        
                    
}

void borrar_particion(std::string &ruta, std::string &nombre){
    //VARIABLES
    FILE *archivo = fopen(ruta.c_str(), "r+b");          //Disco con el que se va a trabajar
    MBR mbr;                                             //MBR para leer el disco
    int posicion = -1;                                   //Posicion de la particion en el mbr
    bool logica = true;                                  //Indica si se va a manejar el borrado en logica 

    //COLOCAR EL PUNTERO DE LECTURA EN LA CABECERA Y LEER MBR
    fseek(archivo, 0, SEEK_SET);
    fread(&mbr, sizeof(MBR), 1, archivo);

    //BUSCAR LA PARTICION EN EL MBR. SI NO ESTA, SE VA A BUSCAR LUEGO EN LA EXTENDIDA (SI EXISTE)
    for(int i = 0; i < 4; i++){
        particion temp = mbr.mbr_partition[i];
        if(temp.part_name == nombre){
            posicion = i;
            logica = false;
            break;
        }
    }

    //=============== PARTICIONES LÓGICAS =============== 
    if(logica){
        //VARIABLES
        int posExtendida;                      //Posicion donde inicia la extendida
        int posAnterior = -1;                  //Posicion del EBR anterior al eliminiado
        int posNext;                           //Posicion del siguiente EBR al eliminado
        EBR ebr;                               //Auxiliar para leer ebr
        bool existe = false;                   //Indica si existe la particion
        bool cabecera_revisada = false;        //Para saber si se esta revisando la cabecera
        bool borrar_cabecera = false;          //Indica si se va a borrar la cabecera
        bool continuar = true;                 //Usado en el ciclo de revisión
        std::string confirmar;                 //Para verificar si se elimina o no  
        char vacio = '\0';                     //Caracter a escribir en el disco 

        //BUSCAR LA EXTENDIDA
        for(int i = 0; i < 4; i++){
            if(mbr.mbr_partition[i].part_type == 'e'){
                posicion = i;
                break;
            }
        }

        if(posicion == -1){
            std::cout << "ERROR: La partición que desea eliminar no existe." << std::endl;
            fclose(archivo);
            return;
        }

        //MOVER EL PUNTERO A LA EXTENDIDA Y LEER EL EBR CABECERA
        posExtendida = mbr.mbr_partition[posicion].part_start;
        fseek(archivo, posExtendida, SEEK_SET);
        fread(&ebr, sizeof(EBR), 1, archivo);

        //BUSCAR LA PARTICION EN LA EXTENDIDA
        while(continuar){
            if(!cabecera_revisada){
                if(ebr.part_name == nombre){
                    existe = true;
                    borrar_cabecera = true;
                    break;
                }

                cabecera_revisada = true;

                if(ebr.part_next == -1){
                    continuar = false;
                }else{
                    posAnterior = posExtendida;
                    posExtendida = ebr.part_next;
                    fseek(archivo, posExtendida, SEEK_SET);
                    fread(&ebr, sizeof(EBR), 1, archivo);
                }
            }else{
                if(ebr.part_name == nombre){
                    existe = true;
                    posNext = ebr.part_next;
                    break;
                }

                if(ebr.part_next == -1){
                    continuar = false;
                }else{
                    posAnterior = posExtendida;
                    posExtendida = ebr.part_next;
                    fseek(archivo, posExtendida, SEEK_SET);
                    fread(&ebr, sizeof(EBR), 1, archivo);
                }
            }
            
        }

        if(!existe){
            std::cout << "ERROR: La partición a borrar no existe. " << std::endl; 
            fclose(archivo);
            return;
        }

        //CONFIRMAR BORRADO
        std::cout << "¿Desea eliminar? [Y/N]" << std::endl;
        getline(std::cin, confirmar);

        if(confirmar == "N" || confirmar == "n"){
            fclose(archivo);
            return;
        }else if(confirmar == "Y" || confirmar == "y"){
            
        }else{
            std::cout <<"ERROR: Entrada inesperada." << std::endl;
            fclose(archivo);
            return;
        }

        //ELIMINAR PARTICION
        if(borrar_cabecera){
            //Borrar la particion
            fseek(archivo, ebr.part_start,SEEK_SET);
            for(int i = 0; i < ebr.part_s; i++){
                fwrite(&vacio, 1,1,archivo);
            }

            //Reiniciar datos en el EBR cabecera
            fseek(archivo, posExtendida, SEEK_SET);
            fread(&ebr, sizeof(EBR), 1, archivo);
            strcpy(ebr.part_name, "");
            ebr.part_s = 0;
            ebr.part_status = '0';
            ebr.part_start = posExtendida + sizeof(EBR);
            fseek(archivo, posExtendida,SEEK_SET);
            fwrite(&ebr, sizeof(EBR), 1, archivo);

            fclose(archivo);
            std::cout << "MENSAJE: Particion " << nombre << " eliminada correctamente." << std::endl;
        }else{
            //Reiniciar datos en el EBR anterior
            fseek(archivo, posAnterior, SEEK_SET);
            fread(&ebr, sizeof(EBR), 1, archivo);
            ebr.part_next = posNext;
            fseek(archivo, posAnterior,SEEK_SET);
            fwrite(&ebr, sizeof(EBR), 1, archivo);

            //Borrar la particion y el ebr
            fseek(archivo, ebr.part_start ,SEEK_SET);
            for(int i = 0; i < ebr.part_s; i++){
                fwrite(&vacio, 1,1,archivo);
            }
            
            fseek(archivo, posExtendida ,SEEK_SET);
            for(int i = 0; i < sizeof(EBR); i++){
                fwrite(&vacio, 1,1,archivo);
            }

            fclose(archivo);
            std::cout << "MENSAJE: Particion " << nombre << " eliminada correctamente." << std::endl;
        }
        
    }

    //=============== PARTICIONES PRIMARIA / EXPANDIDA ===============
    if(!logica){
        //VARIABLES
        std::string confirmar;               //Usada para la confirmación del borrado
        int posParticion;                    //Posicion donde inicia la particion
        char vacio = '\0';                   //Caracter que se va a escribir en el disco

        //CONFIRMAR BORRADO        
        std::cout << "¿Desea eliminar? [Y/N]" << std::endl;
        getline(std::cin, confirmar);

        if(confirmar == "N" || confirmar == "n"){
            fclose(archivo);
            return;
        }else if(confirmar == "Y" || confirmar == "y"){
            
        }else{
            std::cout <<"ERROR: Entrada inesperada." << std::endl;
            fclose(archivo);
            return;
        }

        //MOVER EL PUNTERO A LA PARTICION
        posParticion = mbr.mbr_partition[posicion].part_start;
        fseek(archivo, posParticion, SEEK_SET);

        //BORRAR LA PARTICION
        for(int i = 0; i < mbr.mbr_partition[posicion].part_s; i++){
            fwrite(&vacio, 1, 1, archivo);
        }

        //REESCRIBIR EL MBR
        strcpy(mbr.mbr_partition[posicion].part_name, "");
        mbr.mbr_partition[posicion].part_status = '0';
        mbr.mbr_partition[posicion].part_s = 0;
        mbr.mbr_partition[posicion].part_fit = 'w';
        mbr.mbr_partition[posicion].part_start = 0;
        mbr.mbr_partition[posicion].part_type = 'p';
        fseek(archivo,0,SEEK_SET);                  
        fwrite(&mbr, sizeof(MBR), 1, archivo);
        fclose(archivo);
        std::cout << "MENSAJE: Particion " << nombre << " eliminada correctamente." << std::endl;   
        
    }

}

void modificar_particion(char &tipo, std::string &ruta, int &tamaño, std::string &nombre){
    //VARIABLES
    FILE *archivo = fopen(ruta.c_str(), "r+b");          //Disco con el que se va a trabajar
    bool existe = false;                                 //Indica si existe la particion a modificar.
    int posicion = -1;                                   //Posicion para leer el mbr del disco
    MBR mbr;                    
    char signo;                                          //Diferencia si se esta aumentando o reduciendo espacio
    int espacio;                                        //Almacena el espacio que hay disponible para modificar
    bool logica = true;                                  //Indica si se va a manejar el borrado en logica 

    //COLOCAR EL PUNTERO DE LECTURA EN LA CABECERA
    fseek(archivo, 0, SEEK_SET);
    fread(&mbr, sizeof(MBR), 1, archivo);

    //BUSCAR LA PARTICION. 
    for(int i = 0; i < 4; i++){
        particion temp = mbr.mbr_partition[i];
        if(temp.part_name == nombre){
            posicion = i;
            logica = false;
            break;
        }
    }

    //=============== PARTICIONES LÓGICAS =============== 
    if(logica){
        //VARIABLES
        int posExtendida;                      //Posicion donde inicia la extendida
        int finExtendida;                      //Posicion donde termina la extendida
        EBR ebr;                               //Auxiliar para leer ebr
        bool existe = false;                   //Indica si existe la particion
        bool continuar = true;                 //Usado en el ciclo de revisión 
        int fin;                               //La posicion donde termina el espacio libre

        //BUSCAR LA EXTENDIDA
        for(int i = 0; i < 4; i++){
            if(mbr.mbr_partition[i].part_type == 'e'){
                posicion = i;
                break;
            }
        }

        if(posicion == -1){
            std::cout << "ERROR: La partición que desea modificar no existe." << std::endl;
            fclose(archivo);
            return;
        }

        //MOVER EL PUNTERO A LA EXTENDIDA Y LEER EL EBR CABECERA
        posExtendida = mbr.mbr_partition[posicion].part_start;
        finExtendida = mbr.mbr_partition[posicion].part_start + mbr.mbr_partition[posicion].part_s; 
        fseek(archivo, posExtendida, SEEK_SET);
        fread(&ebr, sizeof(EBR), 1, archivo);

        //BUSCAR LA PARTICION EN LA EXTENDIDA
        while(continuar){
            if(ebr.part_name == nombre){
                existe = true;
                break;
            }

            if(ebr.part_next == -1){
                continuar = false;
            }else{
                posExtendida = ebr.part_next;
                fseek(archivo, posExtendida, SEEK_SET);
                fread(&ebr, sizeof(EBR), 1, archivo);
            }
        }
            
        if(!existe){
            std::cout << "ERROR: La partición a borrar no existe. " << std::endl; 
            fclose(archivo);
            return;
        }

        //DEFINIR SI SE VA A AUMENTAR O REDUCIR LA PARTICION
        if(tamaño > 0){
            signo = '+';
        }else if(tamaño < 0){
            signo = '-';
        }
        tamaño = abs(tamaño);

        //ACTUALIZAR EL ESPACIO
        if(signo == '+'){

            if(ebr.part_next == -1){
                fin = ebr.part_start + ebr.part_s;
                espacio = finExtendida - fin; 
            }else{
                fin = ebr.part_start + ebr.part_s;
                espacio = ebr.part_next - fin;                 
            }

            if(espacio >= tamaño){
                ebr.part_s += tamaño;
                fseek(archivo, posExtendida,SEEK_SET);
                fwrite(&ebr, sizeof(EBR), 1, archivo);
                
                fclose(archivo);
                std::cout << "MENSAJE: Particion " << ebr.part_name << " aumentada correctamente." << std::endl;
            }else{
                std::cout << "ERROR: El espacio que desea agregar supera el espacio existente." << std::endl;
                fclose(archivo);
            }

        }else{
            espacio = ebr.part_s; 
            if(espacio > tamaño){
                ebr.part_s -= tamaño;
                fseek(archivo, posExtendida,SEEK_SET);
                fwrite(&ebr, sizeof(EBR), 1, archivo);
                
                fclose(archivo);
                std::cout << "MENSAJE: Particion " << ebr.part_name << " reducida correctamente." << std::endl;
            }else{
                std::cout << "ERROR: El espacio que desea reducir supera el espacio existente o deja la partición vacía." << std::endl;
                fclose(archivo);
            }
        }
        
    }

    //=============== PARTICIONES PRIMARIA / EXPANDIDA ===============
    if(!logica){
        //VARIABLES
        std::vector<position> posiciones;   
        int indice_posicion;                           //Posicion de la particion en la lista 
        bool next = true;                             //Indica si hay o no una particion despues
        position p1;                                  //Para manejo del espacio a aumentar
        position p2;                                  //Para manejo del espacio a aumentar

        //MARCAR LAS POSICIONES DE LAS PARTICIONES
        for(int i = 0; i < 4; i++){
            if(mbr.mbr_partition[i].part_name[0] != '\0'){
                position temp;
                temp.inicio = mbr.mbr_partition[i].part_start;
                temp.fin = mbr.mbr_partition[i].part_start + mbr.mbr_partition[i].part_s;
                temp.nombre = mbr.mbr_partition[i].part_name;
                posiciones.push_back(temp);
            }
        }

        if(posiciones.size() != 0){
            sort(posiciones.begin(), posiciones.end());
        }

        //DEFINIR SI SE VA A AUMENTAR O REDUCIR LA PARTICION
        if(tamaño > 0){
            signo = '+';
        }else if(tamaño < 0){
            signo = '-';
        }
        tamaño = abs(tamaño);

        //BUSCAR LA PARTICION EN LA LISTA Y DETERMINAR SI ES LA ULTIMA EN EL DISCO
        for(int i = 0; i < posiciones.size(); i++){
            position temp = posiciones[i];
            if(temp.nombre == nombre){
                indice_posicion = i;
                if(i == (posiciones.size() - 1)){
                    next = false;
                }
            }
        }

        //ACTUALIZAR EL ESPACIO
        if(signo == '+'){

            if(next){                                   //Hay una particion despues
                p1 = posiciones[indice_posicion];
                p2 = posiciones[indice_posicion + 1];
                espacio = p2.inicio - p1.fin;
                 
            }else{                                  //Es la ultima particion
                p1 = posiciones[indice_posicion];
                int fin = mbr.mbr_tamano;
                espacio = fin - p1.fin;
            }

            if(espacio >= tamaño){
                mbr.mbr_partition[posicion].part_s += tamaño;
                fseek(archivo,0,SEEK_SET);                  //Posicionarse en byte inicial
                fwrite(&mbr, sizeof(MBR), 1, archivo);
                fclose(archivo);
                std::cout << "MENSAJE: Particion " << nombre << " aumentada correctamente." << std::endl;
            }else{
                std::cout << "ERROR: No se puede añadir espacio a la partición. Espacio insuficiente." << std::endl;
                fclose(archivo);
            } 

        }else{
            position p1 = posiciones[indice_posicion];
            espacio = p1.fin - p1.inicio;
            if(espacio > tamaño){
                mbr.mbr_partition[posicion].part_s -= tamaño;
                fseek(archivo,0,SEEK_SET);                  //Posicionarse en byte inicial
                fwrite(&mbr, sizeof(MBR), 1, archivo);
                fclose(archivo);
                std::cout << "MENSAJE: Particion " << nombre << " reducida correctamente." << std::endl;
            }else{
                std::cout << "ERROR: No se puede reducir espacio a la prtición. Espacio insuficiente." << std::endl;
                fclose(archivo);
            }
        }
        
    }

}