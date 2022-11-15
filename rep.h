#ifndef REP_H
#define REP_H

#include <iostream>
#include <filesystem>
#include <exception>
#include <string>
#include <fstream>
#include <algorithm>
#include <math.h>

#include "structs.h"

void rep(std::vector<std::string> &parametros, std::vector<disco> &discos);

void mbr(std::vector<disco> &discos, int posDisco, std::string &ruta);

void disk(std::vector<disco> &discos, int posDisco, std::string &ruta);

#endif