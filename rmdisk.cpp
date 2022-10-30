#include "rmdisk.h"

void rmdisk(std::vector<std::string> &parametros){
    //VARIABLES
    bool paramFlag = true;                     //Indica si se cumplen con los parametros del comando
    bool required = true;                      //Indica si vienen los parametros obligatorios
    FILE *archivo;                             //Sirve para verificar que el archivo exista        
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

        if(tag == "path"){
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
    if(ruta == ""){
            required = false;
    }

    if(!required){
        std::cout << "ERROR: La instrucción rmdisk carece de todos los parametros obligatorios." << std::endl;
    }

    //VERIFICAR QUE EL ARCHIVO EXISTA
    archivo = fopen(ruta.c_str(), "r");

    if(archivo == NULL){
        std::cout << "ERROR: El disco que desea eliminar no existe." << std::endl;
        return;
    }else{
        fclose(archivo);                 
    }

    //BORRAR EL DISCO
    try {
        std::filesystem::remove(ruta); 
    }
    catch (std::exception& e) { 

    }
    std::cout << "MENSAJE: Archivo eliminado correctamente." << std::endl;
    
}