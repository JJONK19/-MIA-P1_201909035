#include "analizador.h"

void ejecutar(std::string &cadena){
    //VARIABLES
    std::vector<std::string> parametros; 

    //ANALIZAR LA CADENA
    analizar(cadena, parametros);

    //SEPARAR INSTRUCCIÓN 
    if(!parametros.empty()){
        std::string &tipo = parametros[0];

        //Pasar a minusculas
        std::transform(tipo.begin(), tipo.end(), tipo.begin(),[](unsigned char c){
            return tolower(c);
        });

        if(tipo == "mkdisk"){
            std::cout << "Esto funciona" << std::endl;
        }else if(tipo == "rmdisk"){
            
        }else if(tipo == "fdisk"){
            
        }else if(tipo == "mount"){
            
        }else if(tipo == "unmount"){
            
        }else if(tipo == "mkfs"){
            
        }else if(tipo == "login"){
            
        }else if(tipo == "logout"){
            
        }else if(tipo == "mkgrp"){
            
        }else if(tipo == "rmgrp"){
            
        }else if(tipo == "mkusr"){
            
        }else if(tipo == "rmusr"){
            
        }else if(tipo == "chmod"){
            
        }else if(tipo == "mkfile"){
            
        }else if(tipo == "cat"){
            
        }else if(tipo == "remove"){
            
        }else if(tipo == "edit"){
            
        }else if(tipo == "rename"){
            
        }else if(tipo == "mkdir"){
            
        }else if(tipo == "copy"){
            
        }else if(tipo == "move"){
            
        }else if(tipo == "find"){
            
        }else if(tipo == "chown"){
            
        }else if(tipo == "chgrp"){
            
        }else if(tipo == "pause"){
            
        }else if(tipo == "recovery"){
            
        }else if(tipo == "loss"){
            
        }else if(tipo == "exec"){
            
        }else if(tipo == "rep"){
            
        }else{
            std::cout << "Error: El comando ingresado no existe." << std::endl;
        }
    }
}

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