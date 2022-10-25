#ifndef MKDISK_H
#define MKDISK_H

#include <iostream>
#include <exception>
#include <filesystem>

#include "structs.h"

/*
    parametros: Es el vector con los parametros de la instrucción
*/
void mkdisk(std::vector<std::string> &parametros);

#endif