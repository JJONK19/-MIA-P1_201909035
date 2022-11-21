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

void sb(std::vector<disco> &discos, int posDisco, int posParticion, std::string &ruta);

void inode(std::vector<disco> &discos, int posDisco, int posParticion, std::string &ruta);

void block(std::vector<disco> &discos, int posDisco, int posParticion, std::string &ruta);

void journaling(std::vector<disco> &discos, int posDisco, int posParticion, std::string &ruta);

void bm_inode(std::vector<disco> &discos, int posDisco, int posParticion, std::string &ruta);

void bm_block(std::vector<disco> &discos, int posDisco, int posParticion, std::string &ruta);

void tree(std::vector<disco> &discos, int posDisco, int posParticion, std::string &ruta);

void leer_inodo(std::string &ruta, int &posInodos, int &posBloques,int &no_inodo, std::string &codigo, std::string &padre);

void leer_carpeta(std::string &ruta, int &posInodos, int &posBloques, int &no_bloque, std::string &codigo, std::string &padre);

void leer_archivo(std::string &ruta, int &posInodos, int &posBloques, int &no_bloque, std::string &codigo, std::string &padre);

void leer_apuntador(std::string &ruta, int &posInodos, int &posBloques, int &no_bloque, std::string &codigo, std::string &padre, int grado, char &tipo);

#endif