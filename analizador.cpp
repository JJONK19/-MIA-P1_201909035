#include "analizador.h"

void ejecutar(std::string &cadena, usuario &sesion, std::vector<disco> &discos){
    //VARIABLES
    std::vector<std::string> parametros; 

    //ANALIZAR LA CADENA
    analizar(cadena, parametros);

    //IGNORAR COMENTARIOS 
    if(parametros.empty()){
       return; 
    }

    //EJECUTAR INSTRUCCION
    std::string &tipo = parametros[0];

    std::transform(tipo.begin(), tipo.end(), tipo.begin(),[](unsigned char c){
        return tolower(c);
    });

    if(tipo == "mkdisk"){
        //mkdisk();
    }else if(tipo == "rmdisk"){
        //rmdisk();
    }else if(tipo == "fdisk"){
        //fdisk();
    }else if(tipo == "mount"){
        //mount();
    }else if(tipo == "unmount"){
        //unmount();
    }else if(tipo == "mkfs"){
        //mkfs();
    }else if(tipo == "login"){
        //login();
    }else if(tipo == "logout"){
        //logout();
    }else if(tipo == "mkgrp"){
        //mkgrp();
    }else if(tipo == "rmgrp"){
        //rmgrp();
    }else if(tipo == "mkusr"){
        //mkusr();
    }else if(tipo == "rmusr"){
        //rmusr();
    }else if(tipo == "chmod"){
        //chmod();
    }else if(tipo == "mkfile"){
        //mkfile();
    }else if(tipo == "cat"){
        //cat();
    }else if(tipo == "remove"){
        //remove();
    }else if(tipo == "edit"){
        //edit();
    }else if(tipo == "rename"){
        //rename();
    }else if(tipo == "mkdir"){
        //mkdir();
    }else if(tipo == "copy"){
        //copy();
    }else if(tipo == "move"){
        //move();
    }else if(tipo == "find"){
        //find();
    }else if(tipo == "chown"){
        //chown();
    }else if(tipo == "chgrp"){
        //chgrp();
    }else if(tipo == "pause"){
        //pause();
    }else if(tipo == "recovery"){
        //recovery();
    }else if(tipo == "loss"){
        //loss();
    }else if(tipo == "rep"){
        //rep();
    }else if(tipo == "exec"){
        exec(parametros, sesion, discos);
    }else{
        std::cout << "Error: El comando ingresado no existe." << std::endl;
    }
}

