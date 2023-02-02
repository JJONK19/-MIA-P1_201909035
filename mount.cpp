#include "mount.h"

void mount(std::vector<std::string> &parametros, std::vector<disco> &discos){
    //VARIABLES
    bool paramFlag = true;                     //Indica si se cumplen con los parametros del comando
    bool required = true;                      //Indica si vienen los parametros obligatorios
    bool valid = true;                         //Verifica que los valores de los parametros sean correctos
    FILE *archivo;                             //Sirve para verificar que el archivo exista        
    std::string ruta = "";                     //Atributo path
    std::string nombre = "";                   //Atributo name
    int posEBR = -1;                           //Posicion del EBR de la particion 
    int posMBR = -1;                           //Posicion de la particion en el MBR
    int posLogica = -1;                        //Posicion de la logica
    MBR mbr;                                   //Auxiliar para leer el MBR
    EBR ebr;                                   //Auxiliar para leer EBRs
    int tamaño;                                //Tamaño de la particion
    std::string discName;                      //Nombre del disco que contiene la particion 
    int posDisco = -1;                        //Determina la posicion del disco en el vector

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
        std::cout << "ERROR: La instrucción mount carece de todos los parametros obligatorios." << std::endl;
        return;
    }

    //VERIFICAR QUE EL NO ARCHIVO EXISTA
    archivo = fopen(ruta.c_str(), "r");

    if(archivo == NULL){
        std::cout << "ERROR: El disco no existe." << std::endl;
        return;
    }

    //LEER EL MBR
    fseek(archivo, 0,SEEK_SET);
    fread(&mbr, sizeof(MBR), 1, archivo);

    //BUSCAR LA PARTICION. PRIMERO EN EL MBR, LUEGO EN LA EXTENDIDA SI NO ESTÁ
    for(int i = 0; i < 4; i++){
        particion temp = mbr.mbr_partition[i];
        if(temp.part_name == nombre){
            if(temp.part_type != 'e'){
                posMBR = i;
                tamaño = temp.part_s;
                break;
            }else{
                std::cout << "ERROR: No se puede montar una partición extendida." << std::endl;
                fclose(archivo);
                return;
            }
        }
    }

    if(posMBR == -1){
        //Buscar la extendida
        for(int i = 0; i < 4; i++){
            particion temp = mbr.mbr_partition[i];
            if(temp.part_type == 'e'){
                posEBR = temp.part_start;
                break;
            }
        }
        
        //Buscar en la extendida
        if(posEBR == -1){
            std::cout << "ERROR: La partición no existe." << std::endl;
            fclose(archivo);
            return;
        }

        //Leer cabecera de la particion
        fseek(archivo, posEBR, SEEK_SET);
        fread(&ebr, sizeof(EBR), 1, archivo);

        //Determinar si existe la particion
        bool continuar = true;    
        bool existe = false;                               //Indica si existe la particion
        while(continuar){
            //Si encuentra el nombre, termina el proceso
            if(ebr.part_name == nombre){
                existe = true;
                posLogica = posEBR;
                tamaño = ebr.part_s;
                break;
            }

            if(ebr.part_next == -1){
                continuar = false;
            }else{
                posEBR = ebr.part_next;
                fseek(archivo, posEBR, SEEK_SET);
                fread(&ebr, sizeof(EBR), 1, archivo);
            }
        }

        if(!existe){
            std::cout << "ERROR: La partición no existe. " << std::endl; 
            fclose(archivo);
            return;
        }
    }

    //EXTRAER DE LA RUTA EL NOMBRE DEL DISCO
    discName = std::filesystem::path(ruta).stem();

    //MONTAR LA PARTICION
    //1. SI LA LISTA ESTA VACIA, SE AÑADE LA PARTICION Y EL DISCO A LA LISTA
    //2. SI EL DISCO NO EXISTE, SE AÑADE JUNTO A LA PARTICION SIN REVISAR
    //3. SI EL DISCO EXISTE, SE VERIFICA QUE NO ESTE MONTADA LA PARTICION PARA AÑADIRLA

    if(discos.size() != 0){
        //Buscar el disco
        for(int i = 0; i < discos.size(); i++){
            disco temp = discos[i];
            if(temp.nombre == discName){
                posDisco = i;
                break;
            }
        }

        //Añadir el disco y la particion si no existe (Caso 2)
        if(posDisco == -1){
            disco temp;
            temp.nombre = discName;
            temp.ruta = ruta;

            montada nueva;
            nueva.id = "35" + std::to_string(temp.contador) + discName;
            temp.contador++;
            nueva.posEBR = posLogica;
            nueva.posMBR = posMBR;
            nueva.nombre = nombre;
            nueva.tamaño = tamaño;
            temp.particiones.push_back(nueva);
            discos.push_back(temp);
        }

        //Buscar si la particion no existe y montar el disco (Caso 3)
        if(posDisco != -1){
            disco &temp = discos[posDisco];
            for(int i = 0; i < temp.particiones.size(); i++){
                montada t = temp.particiones[i];
                if(t.nombre == nombre){
                    std::cout << "ERROR: La partición " << nombre << " se encuentra montada." << std::endl;
                    fclose(archivo);
                    return;
                }
            }

            montada nueva;
            nueva.id = "35" + std::to_string(temp.contador) + discName;
            temp.contador++;
            nueva.posEBR = posLogica;
            nueva.posMBR = posMBR;
            nueva.nombre = nombre;
            nueva.tamaño = tamaño;
            temp.particiones.push_back(nueva);
        }
    }

    if(discos.size() == 0){
        //Añadir el disco y la particion (Caso 1)
        disco temp;
        temp.nombre = discName;
        temp.ruta = ruta;

        montada nueva;
        nueva.id = "35" + std::to_string(temp.contador) + discName;
        temp.contador++;
        nueva.posEBR = posLogica;
        nueva.posMBR = posMBR;
        nueva.nombre = nombre;
        nueva.tamaño = tamaño;
        temp.particiones.push_back(nueva);
        discos.push_back(temp);
    }


    //ESCRIBIR EL EBR/MBR Y EL SUPERBLOQUE
    if(posLogica == -1){
        //Cambiar el estado de la particion en el MBR
        fseek(archivo, 0,SEEK_SET);
        fread(&mbr, sizeof(MBR), 1, archivo);
        mbr.mbr_partition[posMBR].part_status = '1';

        fseek(archivo, 0,SEEK_SET);
        fwrite(&mbr, sizeof(MBR), 1, archivo);

        //Actualizar el registro del Super Bloque
        sbloque bloque;
        fseek(archivo, mbr.mbr_partition[posMBR].part_start, SEEK_SET);
        fread(&bloque, sizeof(sbloque), 1, archivo);

        if(bloque.s_filesystem_type == 2 || bloque.s_filesystem_type == 3){
            bloque.s_mnt_count++;
            bloque.s_mtime = time(NULL);
            fseek(archivo, mbr.mbr_partition[posMBR].part_start, SEEK_SET);
            fwrite(&bloque, sizeof(sbloque), 1, archivo);
        }

        std::cout << "MENSAJE: Particion montada correctamente." << std::endl;
    }else{
        //Cambiar el estado de la particion en el MBR
        fseek(archivo, posLogica, SEEK_SET);
        fread(&ebr, sizeof(EBR), 1, archivo);
        ebr.part_status = '1';

        fseek(archivo, posLogica,SEEK_SET);
        fwrite(&ebr, sizeof(EBR), 1, archivo);

        //Actualizar el registro del Super Bloque
        sbloque bloque;
        fseek(archivo, ebr.part_s, SEEK_SET);
        fread(&bloque, sizeof(sbloque), 1, archivo);

        if(bloque.s_filesystem_type == 2 || bloque.s_filesystem_type == 3){
            bloque.s_mnt_count++;
            bloque.s_mtime = time(NULL);
            fseek(archivo, ebr.part_s, SEEK_SET);
            fwrite(&bloque, sizeof(sbloque), 1, archivo);
        }

        std::cout << "MENSAJE: Particion montada correctamente." << std::endl;
    }
    fclose(archivo);
    
}