#ifndef EXEC_H
#define EXEC_H

#include <iostream>
#include <iterator>
#include <regex>
#include <fstream>

#include "lexico.h"
#include "structs.h"
#include "mkdisk.h"
#include "rmdisk.h"
#include "fdisk.h"
#include "mount.h"
#include "unmount.h"
#include "mkfs.h"
#include "login.h"
#include "logout.h"
#include "mkgrp.h"
#include "rep.h"


/*
    parametros: lista de parametros de la instrucción exec
    sesion: struct con los datos del usuario loggeado- Sirve para las instrucciones del script. Es global durante la ejecución.
    discos: Vector con la informacón de los discos cargados en memoria. Es global durante la ejecución.
*/
void exec(std::vector<std::string> &parametros, usuario &sesion, std::vector<disco> &discos);

#endif