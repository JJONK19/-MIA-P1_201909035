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
        mkdisk(parametros);
    }else if(tipo == "rmdisk"){
        rmdisk(parametros);
    }else if(tipo == "fdisk"){
        fdisk(parametros);
    }else if(tipo == "mount"){
        mount(parametros, discos);
    }else if(tipo == "unmount"){
        unmount(parametros, discos);
    }else if(tipo == "mkfs"){
        mkfs(parametros, discos);
    }else if(tipo == "login"){
        login(parametros, discos, sesion);
    }else if(tipo == "logout"){
        logout(sesion);
    }else if(tipo == "mkgrp"){
        mkgrp(parametros, discos, sesion);
    }else if(tipo == "rmgrp"){
        rmgrp(parametros, discos, sesion);
    }else if(tipo == "mkusr"){
        mkusr(parametros, discos, sesion);
    }else if(tipo == "rmusr"){
        rmusr(parametros, discos, sesion);
    }else if(tipo == "chmod"){
        chmod(parametros, discos, sesion);
    }else if(tipo == "mkfile"){
        mkfile(parametros, discos, sesion);
    }else if(tipo == "cat"){
        //cat();
    }else if(tipo == "remove"){
        //remove();
    }else if(tipo == "edit"){
        //edit();
    }else if(tipo == "rename"){
        //rename();
    }else if(tipo == "mkdir"){
        mkdir(parametros, discos, sesion);
    }else if(tipo == "copy"){
        //copy();
    }else if(tipo == "move"){
        //move();
    }else if(tipo == "find"){
        //find();
    }else if(tipo == "chown"){
        //chown();
    }else if(tipo == "chgrp"){
        chgrp(parametros, discos, sesion);
    }else if(tipo == "pause"){
        pause();
    }else if(tipo == "recovery"){
        //recovery();
    }else if(tipo == "loss"){
        //loss();
    }else if(tipo == "rep"){
        rep(parametros, discos);
    }else if(tipo == "exec"){
        exec(parametros, sesion, discos);
    }else{
        std::cout << "Error: El comando ingresado no existe." << std::endl;
    }
}

