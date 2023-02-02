#include "mkdisk.h"

void mkdisk(std::vector<std::string> &parametros){
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
            tamaño = std::stoi(value);
        }else if(tag == "f"){
            fit = value;
        }else if(tag == "u"){
            unidad = value;
        }else if(tag == "path"){
            ruta = value;
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
    if(tamaño == 0 || ruta == ""){
            required = false;
    }

    if(!required){
        std::cout << "ERROR: La instrucción mkdisk carece de todos los parametros obligatorios." << std::endl;
    }

    //VALIDACION DE PARAMETROS
    transform(fit.begin(), fit.end(), fit.begin(),[](unsigned char c){
        return tolower(c);
    });

    transform(unidad.begin(), unidad.end(), unidad.begin(),[](unsigned char c){
        return tolower(c);
    });

    if(tamaño <= 0){
        std::cout << "ERROR: El tamaño debe de ser mayor que 0." << std::endl;
        valid = false;
    }

        
    if(fit == "bf" || fit == "ff" || fit == "wf" || fit == ""){

    }else{
        std::cout << "ERROR: Tipo de Fit Invalido." << std::endl;
        valid = false;
    }

    
    if(unidad == "k" || unidad == "m" || unidad == ""){

    }else{
        std::cout << "ERROR: Tipo de Unidad Invalido." << std::endl;
        valid = false;
    }

    if(!valid){
        return;
    }

    //PREPARACIÓN DE PARAMETROS - Determinar el alias del fit y pasar a bytes el tamaño
    if(fit == "" || fit == "ff"){
        fit_char = 'f';
    }else if(fit == "bf"){
        fit_char = 'b';
    }else if(fit == "wf"){
        fit_char = 'w';
    }

    if(unidad == "" || unidad == "m"){
        tamaño = tamaño * 1024 * 1024;
    }else{
        tamaño = tamaño * 1024;
    }

    //VERIFICAR QUE EL NO ARCHIVO EXISTA
    if(archivo = fopen(ruta.c_str(), "r")){
        std::cout << "ERROR: El archivo que desea crear ya existe." << std::endl;
        fclose(archivo);
        return;
    }

    //CREAR DIRECTORIOS EN CASO NO EXISTAN 
    //Creacion: https://stackoverflow.com/questions/43940515/create-a-directory-for-every-element-of-a-path-if-it-does-not-exist
    //Borrado: https://stackoverflow.com/questions/734717/how-to-delete-a-folder-in-c
    try {
        std::filesystem::create_directories(ruta);       
        std::filesystem::remove(ruta);                   
    }
    catch (std::exception &e) { 

    }
    
    //CREAR EL ARCHIVO BINARIO (DISCO) Y LLENARLO DE 0s
    archivo = fopen(ruta.c_str(), "wb");
    for(int i = 0; i < tamaño; i++){
        fwrite(&vacio, 1,1,archivo);
    }
    
    //CREAR EL MBR Y LLENARLO CON VALORES DEFAULT
    MBR mbr;
    mbr.mbr_tamano = tamaño;
    mbr.mbr_dsk_signature = rand()%500;
    mbr.mbr_fecha_creacion = time(NULL);
    mbr.dsk_fit = fit_char;
    for(int i = 0; i < 4; i++){
        strcpy(mbr.mbr_partition[i].part_name, "");
        mbr.mbr_partition[i].part_status = '0';
        mbr.mbr_partition[i].part_s = 0;
        mbr.mbr_partition[i].part_fit = fit_char;
        mbr.mbr_partition[i].part_start = tamaño;  
    }

    //ESCRIBIR EL STRUCT EN EL DISCO
    fseek(archivo, 0, SEEK_SET);                  //El MBR está al inicio del archivo
    fwrite(&mbr, sizeof(MBR), 1, archivo);
    fclose(archivo);
    std::cout << "MENSAJE: Archivo creado correctamente." << std::endl;
}