#ifndef RECOVERY
#define RECOVERY

#include <string>
#include <algorithm>
#include <vector>
#include <iostream>

#include "structs.h"
#include "mkgrp.h"
#include "mkusr.h"
#include "rmusr.h"
#include "rmgrp.h"
#include "chgrp.h"
#include "chmod.h"
#include "mkdir.h"
#include "mkfile.h"
#include "chown.h"
#include "edit.h"
#include "remove.h"
#include "copy.h"
#include "move.h"
#include "rename.h"

void recovery(std::vector<std::string> &parametros, std::vector<disco> &discos);      

#endif
