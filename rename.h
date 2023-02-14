#ifndef RENAME
#define RENAME

#include <iostream>
#include <regex>
#include <stack>
#include <string.h>

#include "structs.h"
//Lineas es el texto del archivo de usuarios
//La ruta es la ubicacion fisica del disco

void rename(std::vector<std::string> &parametros, std::vector<disco> &discos, usuario &sesion);

#endif