#include "unmount.h"

void unmount(std::vector<std::string> &parametros, std::vector<disco> &discos){
    //VARIABLES
    bool paramFlag = true;                     //Indica si se cumplen con los parametros del comando
    bool required = true;                      //Indica si vienen los parametros obligatorios
    FILE *archivo;                             //Sirve para verificar que el archivo exista       
    std::string id = "";                       //Atributo id
    std::string diskName;                      //Nombre del disco
    int posDisco = -1;                         //Posicion del disco en el vector
    int posParticion = -1;                     //Posicion de la particion en la lista del disco

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

        if(tag == "id"){
            id = value;
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
    if(id == ""){
        required = false;
    }

    if(!required){
        std::cout << "ERROR: La instrucción unmount carece de todos los parametros obligatorios." << std::endl;
        return;
    }

    //REMOVER LOS NUMEROS DEL ID PARA OBTENER EL NOMBRE DEL DISCO
    int posicion = 0;
    for(int i = 0; i < id.length(); i++){
        if(isdigit(id[i])){
            posicion++;
        }else{
            break;
        }
    }
    diskName = id.substr(posicion, id.length()-1);

    //BUSCAR EL DISCO 
    for(int i = 0; i < discos.size(); i++){
        disco temp = discos[i];
        if(temp.nombre == diskName){
            posDisco = i;
            break;
        }
    }

    if(posDisco == -1){
        std::cout << "ERROR: No se puede desmontar una partición si el disco no existe." << std::endl;
        return;
    }

    //BUSCAR LA POSICION DE LA PARTICION
    disco &tempD = discos[posDisco];
    for(int i = 0; i < tempD.particiones.size(); i++){
        montada temp = tempD.particiones[i];
        if(temp.id == id){
            posParticion = i;
            break;
        }
    }

    if(posParticion == -1){
        std::cout << "ERROR: La particion que desea eliminar no existe." << std::endl;
        return;
    }

    //DETERMINAR SI EXISTE EL DISCO
    montada &borrar = tempD.particiones[posParticion];
    archivo = fopen(tempD.ruta.c_str(), "r+b");

    if(archivo == NULL){
        std::cout << "ERROR: El disco fisico no existe." << std::endl;
        return;
    }
    
    //REESCRIBIR EL MBR/EBR
    if(borrar.posEBR == -1){
        MBR mbr;
        fseek(archivo, 0,SEEK_SET);
        fread(&mbr, sizeof(MBR), 1, archivo);
        mbr.mbr_partition[borrar.posMBR].part_status = '0';

        fseek(archivo, 0,SEEK_SET);
        fread(&mbr, sizeof(MBR), 1, archivo);

        sbloque bloque;
        fseek(archivo, mbr.mbr_partition[borrar.posMBR].part_start, SEEK_SET);
        fread(&bloque, sizeof(sbloque), 1, archivo);

        if(bloque.s_filesystem_type == 2 || bloque.s_filesystem_type == 3){
            bloque.s_mnt_count++;
            bloque.s_mtime = time(0);
            fseek(archivo, mbr.mbr_partition[borrar.posMBR].part_start, SEEK_SET);
            fwrite(&bloque, sizeof(sbloque), 1, archivo);
        }

    }else{
        EBR ebr;
        fseek(archivo, borrar.posEBR, SEEK_SET);
        fread(&ebr, sizeof(EBR), 1, archivo);
        ebr.part_status = '0';

        fseek(archivo, borrar.posEBR,SEEK_SET);
        fwrite(&ebr, sizeof(EBR), 1, archivo);

        sbloque bloque;
        fseek(archivo, ebr.part_s, SEEK_SET);
        fread(&bloque, sizeof(sbloque), 1, archivo);

        if(bloque.s_filesystem_type == 2 || bloque.s_filesystem_type == 3){
            bloque.s_mnt_count++;
            bloque.s_mtime = time(0);
            fseek(archivo, ebr.part_s, SEEK_SET);
            fwrite(&bloque, sizeof(sbloque), 1, archivo);
        }
        
    }
    //ELIMINAR LA PARTICION DE LA LISTA DEL DISCO
    tempD.particiones.erase(tempD.particiones.begin() + posParticion);
    std::cout << "MENSAJE: Particion desmontada correctamente." << std::endl;
    fclose(archivo);

}