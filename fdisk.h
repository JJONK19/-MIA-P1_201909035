#ifndef FDISK_H
#define FDISK_H

#include <iostream>
#include <vector>
#include <algorithm>

#include "structs.h"

/*
    parametros: Es el vector con los parametros de la instrucción
*/
void fdisk(std::vector<std::string> &parametros);

/*
    tamaño: tamaño de la nueva particion
    tipo: tipo de particion. "l" - logicas, "p" - primaria, "e" - expandida
    ruta: ruta del disco con el que se va a trabajar
    nombre: nombre de la nueva particion a crear
    fit: tipo de fit. b - best, w - worst, f - first
*/
void crear_particion(int &tamaño, char &tipo, std::string &ruta, std::string &nombre, char &fit);

/*
    tipo: tipo de particion. "l" - logicas, "p" - primaria, "e" - expandida
    ruta: ruta del disco con el que se va a trabajar
*/
void borrar_particion(std::string &ruta, std::string &nombre);

/*
    tipo: tipo de particion. "l" - logicas, "p" - primaria, "e" - expandida
    ruta: ruta del disco con el que se va a trabajar
    tamaño: cantidad de bytes que se van a añadir o eliminar de la particion 
*/
void modificar_particion(char &tipo, std::string &ruta, int &tamaño, std::string &nombre);

#endif