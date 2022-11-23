#ifndef ANALIZADOR_H
#define ANALIZADOR_H

#include <string>
#include <algorithm>
#include <vector>
#include <iostream>

#include "lexico.h"
#include "structs.h"
#include "exec.h"
#include "mkdisk.h"
#include "rmdisk.h"
#include "fdisk.h"
#include "mount.h"
#include "unmount.h"
#include "mkfs.h"
#include "login.h"
#include "rep.h"
#include "logout.h"

void ejecutar(std::string &cadena, usuario &sesion, std::vector<disco> &discos);         

#endif