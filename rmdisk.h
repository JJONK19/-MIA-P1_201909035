#ifndef RMDISK_H
#define RMDISK_H

#include <iostream>
#include <exception>
#include <filesystem>

#include "structs.h"

/*
    parametros: Es el vector con los parametros de la instrucción
*/
void rmdisk(std::vector<std::string> &parametros);

#endif