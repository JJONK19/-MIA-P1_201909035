#include "loss.h"

void loss(std::vector<std::string> &parametros, std::vector<disco> &discos){

    //VARIABLES
    bool paramFlag = true;                     //Indica si se cumplen con los parametros del comando
    bool required = true;                      //Indica si vienen los parametros obligatorios
    FILE *archivo;                             //Sirve para verificar que el archivo exista       
    std::string id = "";                       //Atributo id
    std::string diskName;                      //Nombre del disco
    int posDisco = -1;                         //Posicion del disco en el vector
    int posParticion = -1;                     //Posicion de la particion en la lista del disco
    int posInicio;                             //Posicion donde inicia la particion
    int posLectura;                            //Para determinar la posicion de lectura en disco
    sbloque sblock;                            //Para leer el superbloque
    char c = '\0';

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
        std::cout << "ERROR: La instrucción loss carece de todos los parametros obligatorios." << std::endl;
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
        if(temp.id == id){
            posParticion = i;
            break;
        }
    }

    if(posParticion == -1){
        std::cout << "ERROR: La particion que desea dañar no existe." << std::endl;
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

    //LLENAR CON 0s EL BITMAP DE INODOS
    fseek(archivo, sblock.s_bm_inode_start, SEEK_SET);
    fwrite(&c, sizeof(char), sblock.s_inodes_count , archivo);

    //ESCRIBIR CON 0s EL BITMAP DE BLOQUES
    fseek(archivo, sblock.s_bm_block_start, SEEK_SET);
    fwrite(&c, sizeof(char), sblock.s_blocks_count, archivo);

    //LLENAR LOS BLOQUES CON ESPACIOS VACIOS
    fseek(archivo ,sblock.s_block_start, SEEK_SET);
    fwrite(&c , sizeof(char), sblock.s_blocks_count * 64 , archivo);

    //LLENAR LOS INODOS CON ESPACIOS VACIOS
    fseek(archivo ,sblock.s_inode_start, SEEK_SET);
    fwrite(&c , sizeof(char), sblock.s_inodes_count * sizeof(inodo), archivo);

    std::cout << "MENSAJE: Sistema dañado correctamente." << std::endl;
    fclose(archivo);

}
