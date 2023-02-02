#include "mkfs.h"

void mkfs(std::vector<std::string> &parametros, std::vector<disco> &discos){
    //VARIABLES
    bool paramFlag = true;                     //Indica si se cumplen con los parametros del comando
    bool required = true;                      //Indica si vienen los parametros obligatorios
    bool valid = true;                         //Verifica que los valores de los parametros sean correctos
    FILE *archivo;                             //Sirve para verificar que el archivo exista
    char vacio = '\0';                         //Usado para escribir el archivo binario
    char cero = '0';                           //Usado para escribir el archivo binario
    std::string tipo = "";                     //Atrubuto -type
    std::string id = "";                       //Atributo -id
    std::string fs = "";                       //Atributo -fs
    std::string diskName = "";                 //Nombre del disco
    int posDisco = -1;                         //Posicion del disco dentro del vector
    int posParticion = -1;                     //Posicion de la particion dentro del vector del disco
    int posInicio;                             //Posicion donde inicia la particion
    int tamaño;                                //Tamaño de la particion que se va a formatear
    sbloque nuevo;                             //Super Bloque nuevo que se va a escribir 
    inodo ninodo;                              //Para el manejo de los inodos
    bcarpetas ncarpeta;                         //Para el manejo de bloques de carpetas
    barchivos narchivo;                        //Para el manejo de bloques de archivo
    int posLectura = -1;                       //Para posiciones en el disco     

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

        if(tag == "type"){
            tipo = value;
        }else if(tag == "id"){
            id = value;
        }else if(tag == "fs"){
            fs = value;
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
        std::cout << "ERROR: La instrucción mkfs carece de todos los parametros obligatorios." << std::endl;
    }

    //VALIDACION DE PARAMETROS
    transform(tipo.begin(), tipo.end(), tipo.begin(),[](unsigned char c){
        return tolower(c);
    });

    transform(fs.begin(), fs.end(), fs.begin(),[](unsigned char c){
        return tolower(c);
    });
    
    if(fs == "2fs" || fs == "3fs" || fs == ""){

    }else{
        std::cout << "ERROR: Tipo de Sistema de Archivos Invalido." << std::endl;
        valid = false;
    }

    if(tipo == "full" || tipo == ""){

    }else{
        std::cout << "ERROR: Tipo de Formateo Invalido." << std::endl;
        valid = false;
    }

    if(!valid){
        return;
    }

    //PREPARACION DE PARAMETROS
    if(fs == ""){
        fs = "2fs";
    }

    //REMOVER NUMEROS DEL ID PARA OBTENER EL NOMBRE DEL DISCO
    int posicion = 0;
    for(int i = 0; i< id.length(); i++){
        if(isdigit(id[i])){
            posicion++;
        }else{
            break;
        }
    }
    diskName = id.substr(posicion, id.length() - 1);

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

    //DETERMINAR LA POSICION DE INICIO Y EL TAMAÑO DE LA PARTICION
    if(formatear.posMBR != -1){
        MBR mbr;
        fseek(archivo, 0,SEEK_SET);
        fread(&mbr, sizeof(MBR), 1, archivo);
        posInicio = mbr.mbr_partition[formatear.posMBR].part_start;
        tamaño = mbr.mbr_partition[formatear.posMBR].part_s;
    }else{
        EBR ebr;
        fseek(archivo, formatear.posEBR, SEEK_SET);
        fread(&ebr, sizeof(EBR), 1, archivo);
        posInicio = ebr.part_start;
        tamaño = ebr.part_s;
    }

    //ESCRIBIR EL SUPERBLOQUE
    if(fs == "2fs"){
        int n = floor((tamaño - sizeof(sbloque))/(196 + sizeof(inodo)));
        nuevo.s_filesystem_type = 2;
        nuevo.s_inodes_count = n;
        nuevo.s_blocks_count = n * 3;
        nuevo.s_free_blocks_count = (n * 3) - 2; //Carpeta y archivo de usuarios
        nuevo.s_free_inodes_count = n - 2;  //Carpeta y texto de usuarios
        nuevo.s_mtime = time(NULL);
        nuevo.s_umtime = time(NULL);
        nuevo.s_mnt_count = 1; //Se encuentra montado actualmente
        nuevo.s_magic = 0xEF53;
        nuevo.s_inode_s = sizeof(inodo);
        nuevo.s_block_s = sizeof(barchivos);
        nuevo.s_firts_ino = posInicio + sizeof(sbloque) + (n * sizeof(char)) + ((n * sizeof(char)) * 3) + (2 * sizeof(inodo));
        nuevo.s_first_blo = posInicio + sizeof(sbloque) + (n * sizeof(char)) + ((n * sizeof(char)) * 3) + (n * sizeof(inodo)) + (sizeof(barchivos) * 2);
        nuevo.s_bm_inode_start = posInicio + sizeof(sbloque);
        nuevo.s_bm_block_start = posInicio + sizeof(sbloque) + (n * sizeof(char));
        nuevo.s_inode_start = posInicio + sizeof(sbloque) + (n * sizeof(char)) + ((n * sizeof(char)) * 3);
        nuevo.s_block_start = posInicio + sizeof(sbloque) + (n * sizeof(char)) + ((n * sizeof(char)) * 3) + (n *sizeof(inodo));

        fseek(archivo, posInicio, SEEK_SET);
        fwrite(&nuevo, sizeof(sbloque), 1, archivo);
        
    }else{
        int n = floor((tamaño - sizeof(sbloque)) / (196 + sizeof(inodo) + sizeof(registro)));
        nuevo.s_filesystem_type = 3;
        nuevo.s_inodes_count = n;
        nuevo.s_blocks_count = n * 3;
        nuevo.s_free_blocks_count = (n * 3) - 2; //Carpeta y archivo de usuarios
        nuevo.s_free_inodes_count = n - 2;  //Carpeta y texto de usuarios
        nuevo.s_mtime = time(NULL);
        nuevo.s_umtime = time(NULL);
        nuevo.s_mnt_count = 1; //Se encuentra montado actualmente
        nuevo.s_magic = 0xEF53;
        nuevo.s_inode_s = sizeof(inodo);
        nuevo.s_block_s = sizeof(barchivos);
        nuevo.s_firts_ino = posInicio + sizeof(sbloque) + (n * sizeof(char)) + ((n * sizeof(char))*3) + (2 * sizeof(inodo)) + (n * sizeof(registro));
        nuevo.s_first_blo = posInicio + sizeof(sbloque) + (n * sizeof(char)) + ((n * sizeof(char)) * 3) + (n * sizeof (inodo)) + (sizeof(barchivos) * 2) + (n * sizeof(registro));
        nuevo.s_bm_inode_start = posInicio + sizeof(sbloque) + (n * sizeof(registro));
        nuevo.s_bm_block_start = posInicio + sizeof (sbloque) + (n * sizeof(char)) + (n * sizeof(registro));
        nuevo.s_inode_start = posInicio + sizeof(sbloque) + (n * sizeof(char)) + ((n * sizeof(char)) * 3) + (n * sizeof(registro));
        nuevo.s_block_start = posInicio + sizeof(sbloque) + (n * sizeof(char)) + ((n * sizeof(char)) * 3) + (n *sizeof (inodo)) + (n * sizeof(registro));

        fseek(archivo, posInicio, SEEK_SET);
        fwrite(&nuevo, sizeof(sbloque), 1, archivo);
    }

    //LLENAR CON 0s EL BITMAP DE INODOS
    fseek(archivo, nuevo.s_bm_inode_start, SEEK_SET);
    fwrite(&cero, sizeof(cero), nuevo.s_inodes_count, archivo);

    //ESCRIBIR CON 0s EL BITMAP DE BLOQUES
    fseek(archivo, nuevo.s_bm_block_start, SEEK_SET);
    fwrite(&cero, sizeof(cero), nuevo.s_blocks_count, archivo);

    //LLENAR LOS BLOQUES CON ESPACIOS VACIOS
    fseek(archivo ,nuevo.s_block_start, SEEK_SET);
    fwrite(&vacio , sizeof(vacio), nuevo.s_blocks_count * sizeof(barchivos) , archivo);

    //LLENAR LOS INODOS CON ESPACIOS VACIOS
    fseek(archivo ,nuevo.s_inode_start, SEEK_SET);
    fwrite(&vacio , sizeof(vacio), nuevo.s_inodes_count * sizeof(inodo), archivo);

    //SOLO PARA EXT3 - Escribir en el journal el archivo de texto y la carpeta
    registro creacion;
    if(nuevo.s_filesystem_type == 3){
        strcpy(creacion.comando ,"mkdir");
        strcpy(creacion.path ,"/");
        strcpy(creacion.nombre ,"");
        strcpy(creacion.contenido ,"");
        creacion.fecha = time(NULL);
        fseek(archivo, posInicio + sizeof(sbloque),SEEK_SET);
        fwrite(&creacion, sizeof(registro), 1, archivo);

        strcpy(creacion.comando ,"mkfile");
        strcpy(creacion.path ,"/");
        strcpy(creacion.nombre ,"users.txt");
        strcpy(creacion.contenido ,"1,G,root\n1,U,root,root,123\n");
        creacion.fecha = time(NULL);
        fseek(archivo, posInicio + sizeof(sbloque) + sizeof(registro) ,SEEK_SET);
        fwrite(&creacion, sizeof(registro), 1, archivo);
    }

    //MARCAR EL PRIMER INODO
    cero = '1';
    fseek(archivo, nuevo.s_bm_inode_start, SEEK_SET);
    fwrite(&cero, sizeof(cero), 1, archivo);

    //MARCAR EL PRIMER BLOQUE
    char c = 'c';
    fseek(archivo, nuevo.s_bm_block_start, SEEK_SET);
    fwrite(&c, sizeof(c), 1, archivo);

    //CREAR Y ESCRIBIR EL INODO
    ninodo.i_uid = 1;
    ninodo.i_gid = 1;
    ninodo.i_s = 0;
    ninodo.i_atime = time(NULL);
    ninodo.i_ctime = time(NULL);
    ninodo.i_mtime = time(NULL);
    for(int i = 0; i < 15; i++){
        ninodo.i_block[i] = -1;
    }
    ninodo.i_block[0] = 0;
    ninodo.i_type = '0';
    ninodo.i_perm = 777;
    fseek(archivo, nuevo.s_inode_start, SEEK_SET);
    fwrite(&ninodo, sizeof(inodo), 1, archivo);

    //CREAR E INICIAR EL BLOQUE DE CARPETAS
    for(int i = 0; i < 4; i++){
        strcpy(ncarpeta.b_content[i].b_name, "-");
        ncarpeta.b_content[i].b_inodo = -1;
    }

    //REGISTRAR EL INODO ACTUAL Y EL DEL PADRE
    strcpy(ncarpeta.b_content[0].b_name, ".");
    ncarpeta.b_content[0].b_inodo = 0;

    strcpy(ncarpeta.b_content[1].b_name, "..");
    ncarpeta.b_content[1].b_inodo = 0;

    //REGISTRAR EL ARCHIVO DE USUARIOS
    strcpy(ncarpeta.b_content[2].b_name, "users.txt");
    ncarpeta.b_content[2].b_inodo = 1;
    fseek(archivo, nuevo.s_block_start, SEEK_SET);
    fwrite(&ncarpeta, sizeof(bcarpetas), 1, archivo);
    
    //MARCAR UN NUEVO INODO PARA EL ARCHIVO 
    posLectura = nuevo.s_bm_inode_start + sizeof(cero);
    fseek(archivo, posLectura, SEEK_SET);
    fwrite(&cero, sizeof(cero), 1, archivo);

    //MARCAR UN NUEVO BLOQUE PAERA EL ARCHIVO
    char a = 'a';
    posLectura = nuevo.s_bm_block_start + sizeof(a);
    fseek(archivo, posLectura, SEEK_SET);
    fwrite(&a, sizeof(a), 1, archivo);

    //LLENAR EL INODO DEL ARCHIVO
    std::string contenido = "1,G,root\n1,U,root,root,123\n";
    ninodo.i_uid = 1;
    ninodo.i_gid = 1;
    ninodo.i_s = contenido.size();
    ninodo.i_atime = time(NULL);
    ninodo.i_ctime = time(NULL);
    ninodo.i_mtime = time(NULL);
    for(int i = 0; i < 15; i++){
        ninodo.i_block[i] = -1;
    }
    ninodo.i_block[0] = 1;
    ninodo.i_type = '1';
    ninodo.i_perm = 777;
    posLectura = nuevo.s_inode_start + sizeof(inodo);
    fseek(archivo, posLectura, SEEK_SET);
    fwrite(&ninodo, sizeof(inodo), 1, archivo);
    
    //LLENAR Y ESCRIBIR EL BLOQUE DE ARCHIVOS
    strcpy(narchivo.b_content, contenido.c_str());
    posLectura = nuevo.s_block_start + sizeof (barchivos);
    fseek(archivo, posLectura, SEEK_SET);
    fwrite(&narchivo, sizeof(barchivos), 1, archivo);
    
    std::cout << "MENSAJE: Particion formateada correctamente." << std::endl;
    fclose(archivo);
}