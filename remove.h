#ifndef REMOVE
#define REMOVE

#include <iostream>
#include <regex>
#include <stack>
#include <string.h>

#include "structs.h"

//Lineas es el texto del archivo de usuarios
//La ruta es la ubicacion fisica del disco

void remove(std::vector<std::string> &parametros, std::vector<disco> &discos, usuario &sesion);

bool carpetas(usuario &sesion, sbloque &sblock, std::string &ruta, std::vector<int> &borrar_bloques, std::vector<int> &borrar_inodos, int &no_inodo);

bool archivos(usuario &sesion, sbloque &sblock, std::string &ruta, std::vector<int> &borrar_bloques, int &no_inodo);

void limpiar_carpeta(sbloque &sblock, std::string &ruta, std::vector<int> &borrar_bloques, int &no_inodo);

#endif