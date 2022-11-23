#include "logout.h"

void logout(usuario &sesion){
    //VERIFICAR QUE NO EXISTA UNA SESIÓN
    if(sesion.user[0] == '\0'){
        std::cout << "ERROR: No hay una sesión iniciada." << std::endl;
        return;
    }

    //CERRAR LA SESION
    sesion.user = "";
    sesion.pass = "";
    sesion.disco = "";
    sesion.grupo = "";
    std::cout << "MENSAJE: Sesión finalizada." << std::endl;
}