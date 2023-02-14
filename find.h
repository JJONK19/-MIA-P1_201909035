#ifndef FIND
#define FIND

#include <iostream>
#include <regex>
#include <string.h>

#include "structs.h"

//La ruta es la ubicacion fisica del disco

void find(std::vector<std::string> &parametros, std::vector<disco> &discos, usuario &sesion);

void buscar(usuario &sesion, sbloque &sblock, std::string &ruta, int &no_inodo, std::vector<std::string> &carpetas, std::regex &nombre);

#endif