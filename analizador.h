#ifndef ANALIZADOR
#define ANALIZADOR

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
#include "mkgrp.h"
#include "mkusr.h"
#include "rmusr.h"
#include "rmgrp.h"
#include "chgrp.h"
#include "chmod.h"
#include "mkdir.h"
#include "mkfile.h"
#include "pause.h"
#include "loss.h"
#include "chown.h"
#include "edit.h"


void ejecutar(std::string &cadena, usuario &sesion, std::vector<disco> &discos);         

#endif