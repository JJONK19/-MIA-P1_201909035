#ifndef MOVE
#define MOVE

#include <iostream>
#include <regex>
#include <stack>
#include <string.h>

#include "structs.h"
#include "remove.h"
//Lineas es el texto del archivo de usuarios
//La ruta es la ubicacion fisica del disco

void move(std::vector<std::string> &parametros, std::vector<disco> &discos, usuario &sesion);

bool parchivos(usuario &sesion, sbloque &sblock, std::string &ruta, int &no_inodo);

bool pcarpetas(usuario &sesion, sbloque &sblock, std::string &ruta, int &no_inodo);

void actualizar_padre(usuario &sesion, sbloque &sblock, std::string &ruta, int &no_inodo, int &padre_inodo);

#endif