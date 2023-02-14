#ifndef CAT
#define CAT

#include <iostream>
#include <regex>
#include <stack>
#include <string.h>

#include "structs.h"
#include "remove.h"
//Lineas es el texto del archivo de usuarios
//La ruta es la ubicacion fisica del disco

void cat(std::vector<std::string> &parametros, std::vector<disco> &discos, usuario &sesion);

#endif