#include "exec.h"

void exec(std::vector<std::string> &parametros, usuario &sesion, std::vector<disco> &discos){
    //VARIABLES
    std::string ruta = "";          //Almacena la ruta del script
    bool paramFlag = true;          //Indica si se cumplen con los parametros del comando
    bool required = true;           //Indica si vienen los parametros obligatorios
    FILE *archivo;                  //Sirve para verificar que el archivo exista
    std::string linea;              //Usado para leer el script

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
        std::cout << "ERROR: La instrucción exec carece de todos los parametros obligatorios." << std::endl;
    }

    //VERIFICAR QUE EL ARCHIVO EXISTA
    archivo = fopen(ruta.c_str(), "r");

    if(archivo == NULL){
        std::cout << "ERROR: El Script no existe.." << std::endl;
        return;
    }else{
        fclose(archivo);                 //Para leer el archivo se va a utilizar otra forma
    }

    //LEER EL ARCHIVO Y EJECUTAR LAS INSTRUCCIONES
    std::ifstream file(ruta.c_str());
    while (getline(file, linea)){

        //Vaciar el vector. Se va a reutilizar el del parametro.
        parametros.clear();

        //Se debe de imprimir el comando y analizarlo para separar sus parametros 
        std::cout << linea << std::endl;
        analizar(linea, parametros);

        //Si es comentario, parametros viene vacio. Se debe de omitir.
        if(parametros.empty()){
            continue;
        }

        //Determinar el tipo de comando. La posicion 0 contiene el tipo de instruccion. Se pasa a minusculas el tipo.
        std::string &tipo = parametros[0];

        transform(tipo.begin(), tipo.end(), tipo.begin(),[](unsigned char c){
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
            remove(parametros, discos, sesion);
        }else if(tipo == "edit"){
            edit(parametros, discos, sesion);
        }else if(tipo == "rename"){
            //rename();
        }else if(tipo == "mkdir"){
            mkdir(parametros, discos, sesion);
        }else if(tipo == "copy"){
            copy(parametros, discos, sesion);
        }else if(tipo == "move"){
            move(parametros, discos, sesion);
        }else if(tipo == "find"){
            //find();
        }else if(tipo == "chown"){
            chown(parametros, discos, sesion);
        }else if(tipo == "chgrp"){
            chgrp(parametros, discos, sesion);
        }else if(tipo == "pause"){
            pause();
        }else if(tipo == "recovery"){
            //recovery();
        }else if(tipo == "loss"){
            loss(parametros, discos);
        }else if(tipo == "rep"){
            rep(parametros, discos);
        }else{
            std::cout << "Error: El comando ingresado no existe." << std::endl;
        }        

        std::cout << std::endl;
    }

}
