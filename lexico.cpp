#include "lexico.h"

void analizar(std::string &cadena, std::vector<std::string> &parametros){
    //VARIABLES
    cadena += " ";                      //Fin de cadena
    int estado = 0;                             
    std::string temp = "";              //Variable que almacena el contenido de un string 

    //RECORRER LA CADENA
    for (int i = 0; i < cadena.length(); i++) {
        switch(estado){
            case 0:
                if(isalpha(cadena[i])){
                    estado = 1;
                    temp += cadena[i]; 
                }else if(cadena[i] == '-'){
                    estado = 2;
                }else if(cadena[i] == '#'){
                    estado = 3;
                }
                break;

            //Palabras reservadas
            case 1: 
                if(cadena[i] == ' '){
                    parametros.push_back(temp);
                    temp = "";
                    estado = 0;
                }else{
                    temp += cadena[i];
                }
                break;

            //Parametros
            case 2:
                if(cadena[i] == '"'){
                    estado = 21;
                }else if(cadena[i] == ' '){
                    parametros.push_back(temp);
                    temp = "";
                    estado = 0;
                }else{
                    temp += cadena[i];
                }
                break;

            //Reconocer cadenas dentro de parametros
            case 21:
                if(cadena[i] == '"'){
                    parametros.push_back(temp);
                    temp = "";
                    estado = 0;
                }else{
                    temp += cadena[i];
                }
                break;

            //Comentarios
            case 3:
                break;
        }
    }
}