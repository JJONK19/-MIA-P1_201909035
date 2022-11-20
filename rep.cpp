#include "rep.h"

void rep(std::vector<std::string> &parametros, std::vector<disco> &discos){
    //VARIABLES
    bool paramFlag = true;                     //Indica si se cumplen con los parametros del comando
    bool required = true;                      //Indica si vienen los parametros obligatorios
    std::string ruta = "";                     //Atributo path
    std::string nombre = "";                   //Atributo name
    std::string id = "";                       //Atributo ID
    std::string rutaS = "";                    //Atributo ruta 
    std::string diskName;                      //Nombre del disco sin los numeros del ID
    int posDisco = -1;                         //Posicion del disco en la lista
    int posParticion = -1;                     //Posicion de la particion dentro del vector del disco

    //COMPROBACIÓN DE PARAMETROS
    for(int i = 1; i < parametros.size(); i++){
        std::string &temp = parametros[i];
        std::vector<std::string> salida(std::sregex_token_iterator(temp.begin(), temp.end(), flecha, -1),
                    std::sregex_token_iterator());

        std::string &tag = salida[0];
        std::string &value = salida[1];

        //Pasar a minusculas
        transform(tag.begin(), tag.end(), tag.begin(),[](unsigned char c){
            return tolower(c);
        });

        if(tag == "path"){
            ruta = value;
        }else if(tag == "name"){
            nombre = value;
        }else if(tag == "id"){
            id = value;
        }else if(tag == "ruta"){
            rutaS = value;
        }else{
            std::cout << "ERROR: El parametro " << tag << " no es valido." << std::endl;
            paramFlag = false;
            break;
        }
    }

    if(!paramFlag){
        return;
    }

    //COMPROBAR PARAMETROS OBLIGATORIOS
    if(nombre == "0" || ruta == "" || id == ""){
            required = false;
    }

    if(!required){
        std::cout << "ERROR: La instrucción rep carece de todos los parametros obligatorios." << std::endl;
    }

    //REMOVER LOS NUMEROS DEL ID
    int posicion = 0;
    for(int i = 0; i< id.length(); i++){
        if(isdigit(id[i])){
            posicion++;
        }else{
            break;
        }
    }
    diskName = id.substr(posicion, id.length()-1);

    //BUSCAR SI EXISTE EL DISCO EN LA LISTA
    for(int i = 0; i < discos.size(); i++){
        disco temp = discos[i];
        if(temp.nombre == diskName){
            posDisco = i;
            break;
        }
    }

    if(posDisco == -1){
        std::cout << "ERROR: El disco no existe." << std::endl;
        return;
    }

    //BUSCAR LA PARTICION DENTRO DEL DISCO MONTADO
    disco &tempD = discos[posDisco];
    for(int i = 0; i < tempD.particiones.size(); i++){
        montada temp = tempD.particiones[i];
        if(temp.id == id){
            posParticion = i;
            break;
        }
    }

    if(posParticion == -1){
        std::cout << "ERROR: No existe una partción montada con ese ID." << std::endl;
        return;
    }

    //Verificar que existe la carpeta del reporte. Crearla si no.
    //Creacion: https://stackoverflow.com/questions/43940515/create-a-directory-for-every-element-of-a-path-if-it-does-not-exist
    //Borrado: https://stackoverflow.com/questions/734717/how-to-delete-a-folder-in-c
    try {
        std::filesystem::create_directories(ruta);       //Crea los directorios
        std::filesystem::remove(ruta);                   //Borra el dir con el nombre del disco
    }
    catch (std::exception& e) { 

    }
    //SEPARAR TIPO DE REPORTE Y EJECUTARLA
    transform(nombre.begin(), nombre.end(), nombre.begin(),[](unsigned char c){
        return tolower(c);
    });

    if(nombre == "mbr"){
        mbr(discos, posDisco, ruta);
    }else if(nombre == "disk"){
        disk(discos, posDisco, ruta);
    }else if(nombre == "inode"){
        inode(discos, posDisco, posParticion, ruta);
    }else if(nombre == "journaling"){
        journaling(discos, posDisco, posParticion, ruta);
    }else if(nombre == "block"){
        block(discos, posDisco, posParticion, ruta);
    }else if(nombre == "bm_inode"){
        bm_inode(discos, posDisco, posParticion, ruta);
    }else if(nombre == "bm_block"){
        bm_block(discos, posDisco, posParticion, ruta);
    }else if(nombre == "tree"){
        
    }else if(nombre == "sb"){
        sb(discos, posDisco, posParticion, ruta);
    }else if(nombre == "file"){
        
    }else if(nombre == "ls"){
        
    }else{
        std::cout << "ERROR: Tipo de reporte invalido." << std::endl;
    }
}

void mbr(std::vector<disco> &discos, int posDisco, std::string &ruta){
    //VARIABLES
    std::string codigo;             //Contenedor del codigo del dot
    disco &uso = discos[posDisco];  //Disco en uso
    FILE *archivo;                  //Para leer el archivo
    MBR mbr;                        //Para leer el mbr
    int posExtendida;               //Posicion para leer la extendida
    EBR ebr;                        //Para leer los ebr de las particiones logicas          
    std::string comando;            //Instruccion a mandar a la consola para generar el comando

    //VERIFICAR QUE EXISTA EL ARCHIVO
    archivo = fopen(uso.ruta.c_str(), "r+b");
    if(archivo == NULL){
        std::cout << "ERROR: No se encontro el disco." << std::endl;
        return;
    }

    //LEER EL MBR
    fseek(archivo, 0,SEEK_SET);
    fread(&mbr, sizeof(MBR), 1, archivo);

    //ESCRIBIR EL DOT
    codigo = "digraph mbr {node [shape=plaintext] struct1 [label= <<TABLE BORDER='2' CELLBORDER='0' CELLSPACING='0'>";
    
    codigo.append("<TR>");
    codigo.append("<TD BGCOLOR='#cd6155' WIDTH='300'>REPORTE DE MBR</TD>");
    codigo.append("<TD WIDTH='300' BGCOLOR='#cd6155'></TD>");
    codigo.append("</TR>");

    codigo.append("<TR>");
    codigo.append("<TD>Tamaño</TD>");
    codigo.append("<TD>");
    codigo.append(std::to_string(mbr.mbr_tamano));
    codigo.append("</TD>");
    codigo.append("</TR>");       

    codigo.append("<TR>");
    codigo.append("<TD>Fit</TD>");
    codigo.append("<TD>");
    codigo.push_back(mbr.dsk_fit);
    codigo.append("</TD>");
    codigo.append("</TR>");

    codigo.append("<TR>");
    codigo.append("<TD>DSK Signature</TD>");
    codigo.append("<TD>");
    codigo.append(std::to_string(mbr.mbr_dsk_signature));
    codigo.append("</TD>");
    codigo.append("</TR>");

    codigo.append("<TR>");
    codigo.append("<TD>Fecha Creacion</TD>");
    codigo.append("<TD>");
    codigo.append(asctime(gmtime(&mbr.mbr_fecha_creacion)));
    codigo.append("</TD>");
    codigo.append("</TR>");   

    //Particiones    
    for(int i = 0; i < 4; i++){
        if(mbr.mbr_partition[i].part_name[0] != '\0'){
            if(mbr.mbr_partition[i].part_type == 'p'){
                codigo.append("<TR>");
                codigo.append("<TD BGCOLOR='#e67e22' WIDTH='300'>PARTICION</TD>");
                codigo.append("<TD WIDTH='300' BGCOLOR='#e67e22'></TD>");
                codigo.append("</TR>");
        
                codigo.append("<TR>");
                codigo.append("<TD>Nombre</TD>");
                codigo.append("<TD>");
                codigo.append(mbr.mbr_partition[i].part_name);
                codigo.append("</TD>");
                codigo.append("</TR>");

                codigo.append("<TR>");
                codigo.append("<TD>Tamaño</TD>");
                codigo.append("<TD>");
                codigo.append(std::to_string(mbr.mbr_partition[i].part_s));
                codigo.append("</TD>");
                codigo.append("</TR>");

                codigo.append("<TR>");
                codigo.append("<TD>Fit</TD>");
                codigo.append("<TD>");
                codigo.push_back(mbr.mbr_partition[i].part_fit);
                codigo.append("</TD>");
                codigo.append("</TR>");

                codigo.append("<TR>");
                codigo.append("<TD>Inicio</TD>");
                codigo.append("<TD>");
                codigo.append(std::to_string(mbr.mbr_partition[i].part_start));
                codigo.append("</TD>");
                codigo.append("</TR>");
                
                codigo.append("<TR>");
                codigo.append("<TD>Status</TD>");
                codigo.append("<TD>");
                codigo.push_back(mbr.mbr_partition[i].part_status);
                codigo.append("</TD>");
                codigo.append("</TR>");

                codigo.append("<TR>");
                codigo.append("<TD>Tipo</TD>");
                codigo.append("<TD>");
                codigo.push_back(mbr.mbr_partition[i].part_type);
                codigo.append("</TD>");
                codigo.append("</TR>");
            }else{
                codigo.append("<TR>");
                codigo.append("<TD BGCOLOR='#e67e22' WIDTH='300'>PARTICION</TD>");
                codigo.append("<TD WIDTH='300' BGCOLOR='#e67e22'></TD>");
                codigo.append("</TR>");
        
                codigo.append("<TR>");
                codigo.append("<TD>Nombre</TD>");
                codigo.append("<TD>");
                codigo.append(mbr.mbr_partition[i].part_name);
                codigo.append("</TD>");
                codigo.append("</TR>");

                codigo.append("<TR>");
                codigo.append("<TD>Tamaño</TD>");
                codigo.append("<TD>");
                codigo.append(std::to_string(mbr.mbr_partition[i].part_s));
                codigo.append("</TD>");
                codigo.append("</TR>");

                codigo.append("<TR>");
                codigo.append("<TD>Fit</TD>");
                codigo.append("<TD>");
                codigo.push_back(mbr.mbr_partition[i].part_fit);
                codigo.append("</TD>");
                codigo.append("</TR>");

                codigo.append("<TR>");
                codigo.append("<TD>Inicio</TD>");
                codigo.append("<TD>");
                codigo.append(std::to_string(mbr.mbr_partition[i].part_start));
                codigo.append("</TD>");
                codigo.append("</TR>");
                
                codigo.append("<TR>");
                codigo.append("<TD>Status</TD>");
                codigo.append("<TD>");
                codigo.push_back(mbr.mbr_partition[i].part_status);
                codigo.append("</TD>");
                codigo.append("</TR>");

                codigo.append("<TR>");
                codigo.append("<TD>Tipo</TD>");
                codigo.append("<TD>");
                codigo.push_back(mbr.mbr_partition[i].part_type);
                codigo.append("</TD>");
                codigo.append("</TR>"); 

                //Recorrer Logicas
                posExtendida = mbr.mbr_partition[i].part_start; 
                fseek(archivo, posExtendida, SEEK_SET);
                fread(&ebr, sizeof(EBR), 1, archivo);
                bool continuar = true;
                while(continuar){
                    codigo.append("<TR>");
                    codigo.append("<TD BGCOLOR='#f1c40f' WIDTH='300'>PARTICION LOGICA</TD>");
                    codigo.append("<TD WIDTH='300' BGCOLOR='#f1c40f'></TD>");
                    codigo.append("</TR>");
            
                    codigo.append("<TR>");
                    codigo.append("<TD>Nombre</TD>");
                    codigo.append("<TD>");
                    codigo.append(ebr.part_name);
                    codigo.append("</TD>");
                    codigo.append("</TR>");

                    codigo.append("<TR>");
                    codigo.append("<TD>Tamaño</TD>");
                    codigo.append("<TD>");
                    codigo.append(std::to_string(ebr.part_s));
                    codigo.append("</TD>");
                    codigo.append("</TR>");

                    codigo.append("<TR>");
                    codigo.append("<TD>Fit</TD>");
                    codigo.append("<TD>");
                    codigo.push_back(ebr.part_fit);
                    codigo.append("</TD>");
                    codigo.append("</TR>");

                    codigo.append("<TR>");
                    codigo.append("<TD>Inicio</TD>");
                    codigo.append("<TD>");
                    codigo.append(std::to_string(ebr.part_start));
                    codigo.append("</TD>");
                    codigo.append("</TR>");
                    
                    codigo.append("<TR>");
                    codigo.append("<TD>Status</TD>");
                    codigo.append("<TD>");
                    codigo.push_back(ebr.part_status);
                    codigo.append("</TD>");
                    codigo.append("</TR>");

                    codigo.append("<TR>");
                    codigo.append("<TD>Next</TD>");
                    codigo.append("<TD>");
                    codigo.append(std::to_string(ebr.part_next));
                    codigo.append("</TD>");
                    codigo.append("</TR>");
                    
                    if(ebr.part_next == -1){
                        continuar = false;
                    }else{
                        posExtendida = ebr.part_next;
                        fseek(archivo, posExtendida, SEEK_SET);
                        fread(&ebr, sizeof(EBR), 1, archivo);
                    }
                }
            }
        }
    }   
    codigo.append("</TABLE>>];}");
    fclose(archivo);

    //GENERAR EL DOT
    std::ofstream outfile ("grafo.dot");
    outfile << codigo << std::endl;
    outfile.close();

    //REMPLAZAR LA EXTENDION DE LA RUTA   
    //Basado en: https://www.oreilly.com/library/view/c-cookbook/0596007612/ch10s17.html             
    std::string::size_type pos = ruta.rfind('.', ruta.length());
    std::string png = "png";
    if (pos != std::string::npos) {
        ruta.replace(pos+1, png.length(), png);
    }

    //CREAR EL COMANDO DOT
    comando = "dot -Tpng grafo.dot -o";
    comando.append("'");
    comando.append(ruta);
    comando.append("'");

    //GENERAR EL GRAFO
    system(comando.c_str());
    std::cout << "MENSAJE: Reporte MBR creado correctamente." << std::endl;
}

void disk(std::vector<disco> &discos, int posDisco, std::string &ruta){
    //VARIABLES
    std::string codigo;             //Contenedor del codigo del dot
    disco &uso = discos[posDisco];   //Disco en uso
    FILE *archivo;                  //Para leer el archivo
    MBR mbr;                        //Para leer el mbr
    int posExtendida;               //Posicion para leer la extendida
    EBR ebr;                        //Para leer los ebr de las particiones logicas          
    std::string comando;            //Instruccion a mandar a la consola para generar el comando
    float size;                     //Tamaño del disco
    int finExtendida = -1;
    int posEBR = -1;
    std::vector<position> posiciones;
    int porcentaje;                  //Maneja los porcentajes a escribir en el reporte

    //VERIFICAR QUE EXISTA EL ARCHIVO
    archivo = fopen(uso.ruta.c_str(), "r+b");
    if(archivo == NULL){
        std::cout << "ERROR: No se encontro el disco." << std::endl;
        return;
    }

    //LEER EL MBR
    fseek(archivo, 0,SEEK_SET);
    fread(&mbr, sizeof(MBR), 1, archivo);

    //BUSCAR LA EXTENDIDA 
    for(int i = 0; i < 4; i++){
        if(mbr.mbr_partition[i].part_type == 'e'){
            posEBR = mbr.mbr_partition[i].part_start;
            finExtendida = mbr.mbr_partition[i].part_start + mbr.mbr_partition[i].part_s; 
            break;
        }
    }

    //ESCRIBIR EL DOT PARA PARTICIONES PRIMARIAS / EXTENDIDA
    codigo = "digraph mbr {node [shape=plaintext] struct1 [label= <<TABLE BORDER='2' CELLBORDER='1' CELLSPACING='0'>";
    size = (float)mbr.mbr_tamano;

    codigo.append("<TR>");
    codigo.append("<TD ROWSPAN='3' BGCOLOR='#A10035' HEIGHT='100'>MBR</TD>");

    //Crear un lista de las particiones  
    for(int i = 0; i < 4; i++){
        if(mbr.mbr_partition[i].part_name[0] != '\0'){
            position temp;
            temp.inicio = mbr.mbr_partition[i].part_start;
            temp.fin = mbr.mbr_partition[i].part_start + mbr.mbr_partition[i].part_s - 1;
            temp.nombre = mbr.mbr_partition[i].part_name;
            temp.tipo = mbr.mbr_partition[i].part_type;
            temp.tamaño = mbr.mbr_partition[i].part_s;
            posiciones.push_back(temp);
        }
    }

    if(posiciones.size() != 0){
        std::sort(posiciones.begin(), posiciones.end());
    }

    //Añadir el codigo de las particiones y los espacios vacios
    if(posiciones.size() == 0){
        codigo.append("<TD ROWSPAN='3' WIDTH='100' BGCOLOR='#3FA796'>LIBRE<BR/>");
        porcentaje = round((size / size) * 100);
        codigo.append(std::to_string(porcentaje));
        codigo.append("% del disco");
        codigo.append("</TD>");
    }else{
        for(int i = 0; i < posiciones.size(); i++){
            position &x = posiciones[i];
            int free = 0;
            if(i == 0 && i != (posiciones.size()-1)){
                free = x.inicio - EndMBR;
                if(free > 0){
                    codigo.append("<TD ROWSPAN='3' WIDTH='100' BGCOLOR='#3FA796'>LIBRE<BR/>");
                    porcentaje = round((free / size) * 100);
                    codigo.append(std::to_string(porcentaje));
                    codigo.append("% del disco");
                    codigo.append("</TD>");
                }

                //Añadir la particion
                if(x.tipo == 'p'){
                    codigo.append("<TD ROWSPAN='3' BGCOLOR='#355764' WIDTH='100'>PRIMARIA<BR/>");
                    codigo.append(x.nombre);
                    codigo.append("<br/>");
                    porcentaje = round((x.tamaño / size) * 100);
                    codigo.append(std::to_string(porcentaje));
                    codigo.append("% del disco");
                    codigo.append("</TD>");
                    
                }else{
                    codigo.append("<TD COLSPAN ='50' BGCOLOR='#FFA500' WIDTH='100'>EXTENDIDA<BR/>");
                    codigo.append("</TD>");
                }

                //Espacio entre la primera particion y la siguiente
                position &y = posiciones[i + 1];
                free = y.inicio - (x.fin + 1);
                if(free > 0){
                    codigo.append("<TD ROWSPAN='3' WIDTH='100' BGCOLOR='#3FA796'>LIBRE<BR/>");
                    porcentaje = round((free / size) * 100);
                    codigo.append(std::to_string(porcentaje));
                    codigo.append("% del disco");
                    codigo.append("</TD>");
                }
            }else if(i == 0 && i == (posiciones.size()-1)){
                //Espacio entre el inicio y la primera particion
                free = x.inicio - EndMBR;
                if(free > 0){
                    codigo.append("<TD ROWSPAN='3' WIDTH='100' BGCOLOR='#3FA796'>LIBRE<BR/>");
                    porcentaje = round((free / size) * 100);
                    codigo.append(std::to_string(porcentaje));
                    codigo.append("% del disco");
                    codigo.append("</TD>");
                }

                //Añadir la particion
                if(x.tipo == 'p'){
                    codigo.append("<TD ROWSPAN='3' BGCOLOR='#355764' WIDTH='100'>PRIMARIA<BR/>");
                    codigo.append(x.nombre);
                    codigo.append("<br/>");
                    porcentaje = round((x.tamaño / size) * 100);
                    codigo.append(std::to_string(porcentaje));
                    codigo.append("% del disco");
                    codigo.append("</TD>");
                    
                }else{
                    codigo.append("<TD COLSPAN ='50' BGCOLOR='#FFA500' WIDTH='100'>EXTENDIDA<BR/>");
                    codigo.append("</TD>");
                }

                //Espacio entre la primera particion y el fin
                free = size - (x.fin + 1);
                if(free > 0){
                    codigo.append("<TD ROWSPAN='3' WIDTH='100' BGCOLOR='#3FA796'>LIBRE<BR/>");
                    porcentaje = round((free / size) * 100);
                    codigo.append(std::to_string(porcentaje));
                    codigo.append("% del disco");
                    codigo.append("</TD>");
                }
            }else if(i != (posiciones.size()-1)){
                //Añadir la particion
                if(x.tipo == 'p'){
                    codigo.append("<TD ROWSPAN='3' BGCOLOR='#355764' WIDTH='100'>PRIMARIA<BR/>");
                    codigo.append(x.nombre);
                    codigo.append("<br/>");
                    porcentaje = round((x.tamaño / size) * 100);
                    codigo.append(std::to_string(porcentaje));
                    codigo.append("% del disco");
                    codigo.append("</TD>");
                    
                }else{
                    codigo.append("<TD COLSPAN ='50' BGCOLOR='#FFA500' WIDTH='100'>EXTENDIDA<BR/>");
                    codigo.append("</TD>");
                }

                //Espacio entre la primera particion y la siguiente
                position &y = posiciones[i + 1];
                free = y.inicio - (x.fin + 1);
                if(free > 0){
                    codigo.append("<TD ROWSPAN='3' WIDTH='100' BGCOLOR='#3FA796'>LIBRE<BR/>");
                    porcentaje = round((free / size) * 100);
                    codigo.append(std::to_string(porcentaje));
                    codigo.append("% del disco");
                    codigo.append("</TD>");
                }
            }else{
                //Añadir la particion
                if(x.tipo == 'p'){
                    codigo.append("<TD ROWSPAN='3' BGCOLOR='#355764' WIDTH='100'>PRIMARIA<BR/>");
                    codigo.append(x.nombre);
                    codigo.append("<br/>");
                    porcentaje = round((x.tamaño / size) * 100);
                    codigo.append(std::to_string(porcentaje));
                    codigo.append("% del disco");
                    codigo.append("</TD>");
                    
                }else{
                    codigo.append("<TD COLSPAN ='50' BGCOLOR='#FFA500' WIDTH='100'>EXTENDIDA<BR/>");
                    codigo.append("</TD>");
                }

                //Espacio entre la primera particion y el fin
                free = size - (x.fin + 1);
                if(free > 0){
                    codigo.append("<TD ROWSPAN='3' WIDTH='100' BGCOLOR='#3FA796'>LIBRE<BR/>");
                    porcentaje = round((free / size) * 100);
                    codigo.append(std::to_string(porcentaje));
                    codigo.append("% del disco");
                    codigo.append("</TD>");
                }
            }
        }
    }
    codigo.append("</TR>");

    //AÑADIR EL CODIGO DE LAS PARTICIONES LOGICAS
    if(posEBR != -1){
        codigo.append("<TR>");
        fseek(archivo, posEBR, SEEK_SET);
        fread(&ebr, sizeof(EBR), 1, archivo);
        bool cabecera_visitada = false;                    //Indica si es la cabecera la revisada
        bool continuar = true;                            //Sirve para salir del while
        int free;
        while(continuar){
            //Revisar primero la cabecera 
            if(!cabecera_visitada){
                if(ebr.part_s == 0){
                    codigo.append("<TD  BGCOLOR='#1F4690' HEIGHT='100'>EBR</TD>");
                    if(ebr.part_next == -1){
                        free = finExtendida - ebr.part_start; 
                    }else{
                        free = ebr.part_next - ebr.part_start;
                    }

                    if(free > 0){
                        codigo.append("<TD ROWSPAN='3' WIDTH='100' BGCOLOR='#3FA796'>LIBRE<BR/>");
                        porcentaje = floor((free / size) * 100);
                        codigo.append(std::to_string(porcentaje));
                        codigo.append("% del disco");
                        codigo.append("</TD>");
                    }
                }else{
                    codigo.append("<TD  BGCOLOR='#1F4690' HEIGHT='100'>EBR</TD>");
                    codigo.append("<TD  BGCOLOR='#3A5BA0' WIDTH='100'>LOGICA<BR/>");
                    codigo.append(ebr.part_name);
                    codigo.append("<br/>");
                    porcentaje = floor((ebr.part_s / size)*100);
                    codigo.append(std::to_string(porcentaje));
                    codigo.append("% del disco");
                    codigo.append("</TD>");
                    if(ebr.part_next == -1){
                        free = finExtendida - (ebr.part_start + ebr.part_s); 
                    }else{
                        free = ebr.part_next - (ebr.part_start + ebr.part_s);
                    }

                    if(free > 0){
                        codigo.append("<TD ROWSPAN='3' WIDTH='100' BGCOLOR='#3FA796'>LIBRE<BR/>");
                        porcentaje = floor((free / size) * 100);
                        codigo.append(std::to_string(porcentaje));
                        codigo.append("% del disco");
                        codigo.append("</TD>");
                    }
                }

                cabecera_visitada = false;
                if(ebr.part_next == -1){
                    continuar = false;
                }else{
                    posEBR = ebr.part_next;
                    fseek(archivo, posEBR, SEEK_SET);
                    fread(&ebr, sizeof(EBR), 1, archivo);
                }
            }else{
                codigo.append("<TD  BGCOLOR='#1F4690' HEIGHT='100'>EBR</TD>");
                codigo.append("<TD  BGCOLOR='#3A5BA0' WIDTH='100'>LOGICA<BR/>");
                codigo.append(ebr.part_name);
                codigo.append("<br/>");
                porcentaje = floor((ebr.part_s / size)*100);
                codigo.append(std::to_string(porcentaje));
                codigo.append("% del disco");
                codigo.append("</TD>");
                if(ebr.part_next == -1){
                    free = finExtendida - (ebr.part_start + ebr.part_s); 
                }else{
                    free = ebr.part_next - (ebr.part_start + ebr.part_s);
                }

                if(free > 0){
                    codigo.append("<TD ROWSPAN='3' WIDTH='100' BGCOLOR='#3FA796'>LIBRE<BR/>");
                    porcentaje = floor((free / size) * 100);
                    codigo.append(std::to_string(porcentaje));
                    codigo.append("% del disco");
                    codigo.append("</TD>");
                }
                if(ebr.part_next == -1){
                    continuar = false;
                }else{
                    posEBR = ebr.part_next;
                    fseek(archivo, posEBR, SEEK_SET);
                    fread(&ebr, sizeof(EBR), 1, archivo);
                }
            }
        }
        codigo.append("</TR>");
    }
    codigo.append("</TABLE>>];}");
    fclose(archivo);

    //GENERAR EL DOT
    std::ofstream outfile ("grafo.dot");
    outfile << codigo << std::endl;
    outfile.close();

    //REMPLAZAR LA EXTENDION DE LA RUTA   
    //Basado en: https://www.oreilly.com/library/view/c-cookbook/0596007612/ch10s17.html             
    std::string::size_type pos = ruta.rfind('.', ruta.length());
    std::string png = "png";
    if (pos != std::string::npos) {
        ruta.replace(pos+1, png.length(), png);
    }

    //CREAR EL COMANDO DOT
    comando = "dot -Tpng grafo.dot -o";
    comando.append("'");
    comando.append(ruta);
    comando.append("'");

    //GENERAR EL GRAFO
    system(comando.c_str());
    std::cout << "MENSAJE: Reporte DISKS creado correctamente." << std::endl;

}

void sb(std::vector<disco> &discos, int posDisco, int posParticion, std::string &ruta){
    //VARIABLES
    std::string codigo;                                      //Contenedor del codigo del dot
    disco &disc_uso = discos[posDisco];                      //Disco en uso
    montada &part_uso = disc_uso.particiones[posParticion]; //Particion Montada 
    FILE *archivo;                                          //Para leer el archivo
    MBR mbr;                                                //Para leer el mbr
    int posExtendida;                                       //Posicion para leer la extendida
    EBR ebr;                                                //Para leer los ebr de las particiones logicas          
    std::string comando;                                    //Instruccion a mandar a la consola para generar el comando
    int posInicio;                                          //Posicion donde inicia la particion
    sbloque sblock;                                         //Para leer el superbloque

    //VERIFICAR QUE EXISTA EL ARCHIVO
    archivo = fopen(disc_uso.ruta.c_str(), "r+b");
    if(archivo == NULL){
        std::cout << "ERROR: No se encontro el disco." << std::endl;
        return;
    }

    //DETERMINAR LA POSICION DE INICIO PARA LEER LA PARTICION
    if(part_uso.posMBR != -1){
        MBR mbr;
        fseek(archivo, 0,SEEK_SET);
        fread(&mbr, sizeof(MBR), 1, archivo);
        posInicio = mbr.mbr_partition[part_uso.posMBR].part_start;
    }else{
        EBR ebr;
        fseek(archivo, part_uso.posEBR, SEEK_SET);
        fread(&ebr, sizeof(EBR), 1, archivo);
        posInicio = ebr.part_start;
    }

    //LEER EL SUPERBLOQUE
    fseek(archivo, posInicio, SEEK_SET);
    fread(&sblock, sizeof(sbloque), 1, archivo);
    
    //ESCRIBIR EL DOT
    codigo = "digraph mbr {node [shape=plaintext] struct1 [label= <<TABLE BORDER='2' CELLBORDER='0' CELLSPACING='0'>";
    
    codigo.append("<TR>");
    codigo.append("<TD BGCOLOR='#cd6155' WIDTH='300'>REPORTE DE SUPERBLOQUE</TD>");
    codigo.append("<TD WIDTH='300' BGCOLOR='#cd6155'></TD>");
    codigo.append("</TR>");

    codigo.append("<TR>");
    codigo.append("<TD>Tipo de Sistema</TD>");
    codigo.append("<TD>");
    if(sblock.s_filesystem_type == 2){
        codigo.append("EXT2");
    }else{
        codigo.append("EXT3");
    }
    codigo.append("</TD>");
    codigo.append("</TR>");

    codigo.append("<TR>");
    codigo.append("<TD>Posición del Bitmap de Inodos</TD>");
    codigo.append("<TD>");
    codigo.append(std::to_string(sblock.s_bm_inode_start));
    codigo.append("</TD>");
    codigo.append("</TR>");

    codigo.append("<TR>");
    codigo.append("<TD>Tamaño del Inodo</TD>");
    codigo.append("<TD>");
    codigo.append(std::to_string(sblock.s_inode_s));
    codigo.append("</TD>");
    codigo.append("</TR>");

    codigo.append("<TR>");
    codigo.append("<TD>Inicio de los Inodos</TD>");
    codigo.append("<TD>");
    codigo.append(std::to_string(sblock.s_inode_start));
    codigo.append("</TD>");
    codigo.append("</TR>");

    codigo.append("<TR>");
    codigo.append("<TD>Posición del Primer Inodo Libre</TD>");
    codigo.append("<TD>");
    codigo.append(std::to_string(sblock.s_firts_ino));
    codigo.append("</TD>");
    codigo.append("</TR>");

    codigo.append("<TR>");
    codigo.append("<TD>Total de Inodos</TD>");
    codigo.append("<TD>");
    codigo.append(std::to_string(sblock.s_inodes_count));
    codigo.append("</TD>");
    codigo.append("</TR>");

    codigo.append("<TR>");
    codigo.append("<TD>Inodos Libres</TD>");
    codigo.append("<TD>");
    codigo.append(std::to_string(sblock.s_free_inodes_count));
    codigo.append("</TD>");
    codigo.append("</TR>");

    codigo.append("<TR>");
    codigo.append("<TD>Posición del Bitmap de Bloques</TD>");
    codigo.append("<TD>");
    codigo.append(std::to_string(sblock.s_bm_block_start));
    codigo.append("</TD>");
    codigo.append("</TR>");

    codigo.append("<TR>");
    codigo.append("<TD>Tamaño del Bloque</TD>");
    codigo.append("<TD>");
    codigo.append(std::to_string(sblock.s_block_s));
    codigo.append("</TD>");
    codigo.append("</TR>");

    codigo.append("<TR>");
    codigo.append("<TD>Inicio de los Bloques</TD>");
    codigo.append("<TD>");
    codigo.append(std::to_string(sblock.s_block_start));
    codigo.append("</TD>");
    codigo.append("</TR>");

    codigo.append("<TR>");
    codigo.append("<TD>Posición del Primer Bloque Libre</TD>");
    codigo.append("<TD>");
    codigo.append(std::to_string(sblock.s_first_blo));
    codigo.append("</TD>");
    codigo.append("</TR>");

    codigo.append("<TR>");
    codigo.append("<TD>Total de Bloques</TD>");
    codigo.append("<TD>");
    codigo.append(std::to_string(sblock.s_blocks_count));
    codigo.append("</TD>");
    codigo.append("</TR>");
    
    codigo.append("<TR>");
    codigo.append("<TD>Bloques Libres</TD>");
    codigo.append("<TD>");
    codigo.append(std::to_string(sblock.s_free_blocks_count));
    codigo.append("</TD>");
    codigo.append("</TR>");

    codigo.append("<TR>");
    codigo.append("<TD>Ultima fecha - Montado</TD>");
    codigo.append("<TD>");
    codigo.append(asctime(gmtime(&sblock.s_mtime)));
    codigo.append("</TD>");
    codigo.append("</TR>");   

    codigo.append("<TR>");
    codigo.append("<TD>Ultima fecha - Desmontado</TD>");
    codigo.append("<TD>");
    codigo.append(asctime(gmtime(&sblock.s_umtime)));
    codigo.append("</TD>");
    codigo.append("</TR>");   

    codigo.append("<TR>");
    codigo.append("<TD>Veces Montado</TD>");
    codigo.append("<TD>");
    codigo.append(std::to_string(sblock.s_mnt_count));
    codigo.append("</TD>");
    codigo.append("</TR>");

    codigo.append("<TR>");
    codigo.append("<TD>Magic Number</TD>");
    codigo.append("<TD>");
    codigo.append(std::to_string(sblock.s_magic));
    codigo.append("</TD>");
    codigo.append("</TR>");
    
    codigo.append("</TABLE>>];}");
    fclose(archivo);
    

    //GENERAR EL DOT
    std::ofstream outfile ("grafo.dot");
    outfile << codigo << std::endl;
    outfile.close();

    //REMPLAZAR LA EXTENDION DE LA RUTA   
    //Basado en: https://www.oreilly.com/library/view/c-cookbook/0596007612/ch10s17.html             
    std::string::size_type pos = ruta.rfind('.', ruta.length());
    std::string png = "png";
    if (pos != std::string::npos) {
        ruta.replace(pos+1, png.length(), png);
    }

    //CREAR EL COMANDO DOT
    comando = "dot -Tpng grafo.dot -o";
    comando.append("'");
    comando.append(ruta);
    comando.append("'");

    //GENERAR EL GRAFO
    system(comando.c_str());
    std::cout << "MENSAJE: Reporte SB creado correctamente." << std::endl;
}

void inode(std::vector<disco> &discos, int posDisco, int posParticion, std::string &ruta){
    //VARIABLES
    std::string codigo;                                      //Contenedor del codigo del dot
    disco &disc_uso = discos[posDisco];                      //Disco en uso
    montada &part_uso = disc_uso.particiones[posParticion]; //Particion Montada 
    FILE *archivo;                                          //Para leer el archivo
    MBR mbr;                                                //Para leer el mbr
    int posExtendida;                                       //Posicion para leer la extendida
    EBR ebr;                                                //Para leer los ebr de las particiones logicas          
    std::string comando;                                    //Instruccion a mandar a la consola para generar el comando
    int posInicio;                                          //Posicion donde inicia la particion
    sbloque sblock;                                         //Para leer el superbloque
    inodo linodo;                                           //Para leer los inodos
    std::vector<int> inodos_usados;                         //Posiciones del bitmap inodos usados  
    char lectura;                                           //Para leer los caracteres del bitmap  
    int posLectura;                                         //Usado para las posiciones de lectura                         

    //VERIFICAR QUE EXISTA EL ARCHIVO
    archivo = fopen(disc_uso.ruta.c_str(), "r+b");
    if(archivo == NULL){
        std::cout << "ERROR: No se encontro el disco." << std::endl;
        return;
    }

    //DETERMINAR LA POSICION DE INICIO PARA LEER LA PARTICION
    if(part_uso.posMBR != -1){
        MBR mbr;
        fseek(archivo, 0,SEEK_SET);
        fread(&mbr, sizeof(MBR), 1, archivo);
        posInicio = mbr.mbr_partition[part_uso.posMBR].part_start;
    }else{
        EBR ebr;
        fseek(archivo, part_uso.posEBR, SEEK_SET);
        fread(&ebr, sizeof(EBR), 1, archivo);
        posInicio = ebr.part_start;
    }

    //LEER EL SUPERBLOQUE
    fseek(archivo, posInicio, SEEK_SET);
    fread(&sblock, sizeof(sbloque), 1, archivo);
    
    //LEER EL BITMAP Y HACER UNA LISTA DE LOS INODOS USADOS
    for(int i = 0; i < sblock.s_inodes_count; i++){
        posLectura = sblock.s_bm_inode_start + (sizeof(char) * i);
        fseek(archivo, posLectura, SEEK_SET);
        fread(&lectura, sizeof(lectura), 1, archivo);

        if(lectura == '1'){
            inodos_usados.push_back(i);
        }
    }

    //ESCRIBIR EL DOT
    codigo = "digraph G { \n rankdir = LR; node[shape = plaintext]; \n";

    //Declarar los nodos
    for(int i = 0; i < inodos_usados.size(); i++){
        posLectura = sblock.s_inode_start + (sizeof(inodo) * inodos_usados[i]);
        fseek(archivo, posLectura, SEEK_SET);
        fread(&linodo, sizeof(inodo), 1, archivo);

        std::string nombre = "INODO";
        nombre.append(std::to_string(inodos_usados[i]));
        codigo.append(nombre);
        nombre = "Inodo ";
        nombre.append(std::to_string(inodos_usados[i]));
        codigo.append("[ label = <<TABLE BORDER='2' CELLBORDER='0' CELLSPACING='5' BGCOLOR='#0f4c5c'>\n");
        codigo.append("<TR><TD colspan ='2' ><b>");
        codigo.append(nombre);
        codigo.append("</b></TD></TR>\n");

        codigo.append("<TR>");
        codigo.append("<TD Align='left'>");
        codigo.append("ID del Propietario:");
        codigo.append("</TD>");
        codigo.append("<TD>");
        codigo.append(std::to_string(linodo.i_uid));
        codigo.append("</TD>");
        codigo.append("</TR>");

        codigo.append("<TR>");
        codigo.append("<TD Align='left'>");
        codigo.append("ID del Grupo:");
        codigo.append("</TD>");
        codigo.append("<TD>");
        codigo.append(std::to_string(linodo.i_gid));
        codigo.append("</TD>");
        codigo.append("</TR>");

        codigo.append("<TR>");
        codigo.append("<TD Align='left'>");
        codigo.append("Tamaño del archivo:");
        codigo.append("</TD>");
        codigo.append("<TD>");
        codigo.append(std::to_string(linodo.i_s));
        codigo.append("</TD>");
        codigo.append("</TR>");

        codigo.append("<TR>");
        codigo.append("<TD Align='left'>");
        codigo.append("Ultima lectura:");
        codigo.append("</TD>");
        codigo.append("<TD>");
        codigo.append(asctime(gmtime(&linodo.i_atime)));
        codigo.append("</TD>");
        codigo.append("</TR>");


        codigo.append("<TR>");
        codigo.append("<TD Align='left'>");
        codigo.append("Fecha de Creación:");
        codigo.append("</TD>");
        codigo.append("<TD>");
        codigo.append(asctime(gmtime(&linodo.i_ctime)));
        codigo.append("</TD>");
        codigo.append("</TR>");


        codigo.append("<TR>");
        codigo.append("<TD Align='left'>");
        codigo.append("Ultima modificación:");
        codigo.append("</TD>");
        codigo.append("<TD>");
        codigo.append(asctime(gmtime(&linodo.i_mtime)));
        codigo.append("</TD>");
        codigo.append("</TR>");


        for(int j = 0; j < 15; j++){
            codigo.append("<TR>");
            codigo.append("<TD Align='left'>");
            nombre = "Bloque ";
            nombre.append(std::to_string(j));
            nombre.append(":");
            codigo.append(nombre);
            codigo.append("</TD>");
            codigo.append("<TD>");
            codigo.append(std::to_string(linodo.i_block[j]));
            codigo.append("</TD>");
            codigo.append("</TR>");
        }

        codigo.append("<TR>");
        codigo.append("<TD Align='left'>");
        codigo.append("Tipo de Inodo:");
        codigo.append("</TD>");
        codigo.append("<TD>");
        codigo.push_back(linodo.i_type);
        codigo.append("</TD>");
        codigo.append("</TR>");


        codigo.append("<TR>");
        codigo.append("<TD Align='left'>");
        codigo.append("Permisos:");
        codigo.append("</TD>");
        codigo.append("<TD>");
        codigo.append(std::to_string(linodo.i_perm));
        codigo.append("</TD>");
        codigo.append("</TR>");

        codigo.append("</TABLE>>];\n");
    }

    //Unir los nodos
    for(int i = 0; i < inodos_usados.size(); i++){
        std::string nombre = "INODO";
        nombre.append(std::to_string(inodos_usados[i])); 
        codigo.append(nombre);

        if(i != (inodos_usados.size() - 1)){
            codigo.append("->");
        }
    }
    codigo.append("}");
    fclose(archivo);
    
    //GENERAR EL DOT
    std::ofstream outfile ("grafo.dot");
    outfile << codigo << std::endl;
    outfile.close();

    //REMPLAZAR LA EXTENDION DE LA RUTA   
    //Basado en: https://www.oreilly.com/library/view/c-cookbook/0596007612/ch10s17.html             
    std::string::size_type pos = ruta.rfind('.', ruta.length());
    std::string png = "png";
    if (pos != std::string::npos) {
        ruta.replace(pos+1, png.length(), png);
    }

    //CREAR EL COMANDO DOT
    comando = "dot -Tpng grafo.dot -o";
    comando.append("'");
    comando.append(ruta);
    comando.append("'");

    //GENERAR EL GRAFO
    system(comando.c_str());
    std::cout << "MENSAJE: Reporte Inode creado correctamente." << std::endl;
}

void block(std::vector<disco> &discos, int posDisco, int posParticion, std::string &ruta){
    //VARIABLES
    std::string codigo;                                      //Contenedor del codigo del dot
    disco &disc_uso = discos[posDisco];                      //Disco en uso
    montada &part_uso = disc_uso.particiones[posParticion]; //Particion Montada 
    FILE *archivo;                                          //Para leer el archivo
    MBR mbr;                                                //Para leer el mbr
    int posExtendida;                                       //Posicion para leer la extendida
    EBR ebr;                                                //Para leer los ebr de las particiones logicas          
    std::string comando;                                    //Instruccion a mandar a la consola para generar el comando
    int posInicio;                                          //Posicion donde inicia la particion
    sbloque sblock;                                         //Para leer el superbloque
    bcarpetas lcarpeta;                                     //Para leer bloques de carpetas
    barchivos larchivo;                                     //Para leer bloques de archivos
    bapuntadores lapuntador;                                //Para leer bloques de apuntadores
    std::vector<int> bloques_usados;                         //Posiciones del bitmap inodos usados
    std::vector<char> tipos;                                //Caracteres usados en el bitmap de bloques  
    char lectura;                                           //Para leer los caracteres del bitmap  
    int posLectura;                                         //Usado para las posiciones de lectura                         

    //VERIFICAR QUE EXISTA EL ARCHIVO
    archivo = fopen(disc_uso.ruta.c_str(), "r+b");
    if(archivo == NULL){
        std::cout << "ERROR: No se encontro el disco." << std::endl;
        return;
    }

    //DETERMINAR LA POSICION DE INICIO PARA LEER LA PARTICION
    if(part_uso.posMBR != -1){
        MBR mbr;
        fseek(archivo, 0,SEEK_SET);
        fread(&mbr, sizeof(MBR), 1, archivo);
        posInicio = mbr.mbr_partition[part_uso.posMBR].part_start;
    }else{
        EBR ebr;
        fseek(archivo, part_uso.posEBR, SEEK_SET);
        fread(&ebr, sizeof(EBR), 1, archivo);
        posInicio = ebr.part_start;
    }

    //LEER EL SUPERBLOQUE
    fseek(archivo, posInicio, SEEK_SET);
    fread(&sblock, sizeof(sbloque), 1, archivo);
    
    //LEER EL BITMAP Y HACER UNA LISTA DE LOS BLOQUES USADOS Y SUS CARACTERES
    for(int i = 0; i < sblock.s_blocks_count; i++){
        posLectura = sblock.s_bm_block_start + (sizeof(char) * i);
        fseek(archivo, posLectura, SEEK_SET);
        fread(&lectura, sizeof(lectura), 1, archivo);

        if(lectura == 'c' || lectura == 'p' || lectura == 'a'){
            bloques_usados.push_back(i);
            tipos.push_back(lectura);
        }
    }

    //ESCRIBIR EL DOT
    codigo = "digraph G { \n rankdir = LR; node[shape = plaintext];\n";

    //Declarar los nodos
    for(int i = 0; i < bloques_usados.size(); i++){
        posLectura = sblock.s_block_start + (64 * bloques_usados[i]);
        fseek(archivo, posLectura, SEEK_SET);

        //Bloques de Carpetas
        if(tipos[i] == 'c'){
            fread(&lcarpeta, sizeof(bcarpetas), 1, archivo);
            std::string nombre = "BLOQUE";
            nombre.append(std::to_string(bloques_usados[i]));
            codigo.append(nombre);

            nombre = "Bloque Carpetas ";
            nombre.append(std::to_string(bloques_usados[i]));
            codigo.append("[ label = <<TABLE BORDER='2' CELLBORDER='0' CELLSPACING='5' BGCOLOR='#8b8c89'>\n");
            codigo.append("<TR><TD colspan ='2' ><b>");
            codigo.append(nombre);
            codigo.append("</b></TD></TR>\n");
            codigo.append("<TR><TD><b>Nombre</b></TD><TD><b>Inodo</b></TD></TR>"); 

            for(int j = 0; j < 4; j++){
                content &temp = lcarpeta.b_content[j];
                codigo.append("<TR>");
                codigo.append("<TD>");
                codigo.append(temp.b_name);
                codigo.append("</TD>");
                codigo.append("<TD>");
                codigo.append(std::to_string(temp.b_inodo));
                codigo.append("</TD>");
                codigo.append("</TR>");
            }
            codigo.append("</TABLE>>];\n");
        }

        //Bloques de Apuntadores
        if(tipos[i] == 'p'){
            fread(&lapuntador, sizeof(bapuntadores), 1, archivo);
            std::string nombre = "BLOQUE";
            nombre.append(std::to_string(bloques_usados[i]));
            codigo.append(nombre);

            nombre = "Bloque Apuntadores ";
            nombre.append(std::to_string(bloques_usados[i]));
            codigo.append("[ label = <<TABLE BORDER='2' CELLBORDER='0' CELLSPACING='5' BGCOLOR='#9a031e'>\n");
            codigo.append("<TR><TD><b>");
            codigo.append(nombre);
            codigo.append("</b></TD></TR>\n"); 

            for(int j = 0; j < 16; j++){
                codigo.append("<TR>");
                codigo.append("<TD>");
                codigo.append(std::to_string(lapuntador.b_pointers[j]));
                codigo.append("</TD>");
                codigo.append("</TR>");
            }
            codigo.append("</TABLE>>];\n");
        }

        //Bloques de Archivos
        if(tipos[i] == 'a'){
            fread(&larchivo, sizeof(barchivos), 1, archivo);
            std::string nombre = "BLOQUE";
            nombre.append(std::to_string(bloques_usados[i]));
            codigo.append(nombre);

            nombre = "Bloque Archivos ";
            nombre.append(std::to_string(bloques_usados[i]));
            codigo.append("[ label = <<TABLE BORDER='2' CELLBORDER='0' CELLSPACING='5' BGCOLOR='#fb8b24'>\n");
            codigo.append("<TR><TD><b>");
            codigo.append(nombre);
            codigo.append("</b></TD></TR>\n"); 

            codigo.append("<TR>");
            codigo.append("<TD>");
            codigo.append(larchivo.b_content);
            codigo.append("</TD>");
            codigo.append("</TR>");
            codigo.append("</TABLE>>];\n");
        }
    }

    //Unir los nodos
    for(int i = 0; i < bloques_usados.size(); i++){
        std::string nombre = "BLOQUE";
        nombre.append(std::to_string(bloques_usados[i])); 
        codigo.append(nombre);

        if(i != (bloques_usados.size() - 1)){
            codigo.append("->");
        }
    }
    codigo.append("}");
    fclose(archivo);
    
    //GENERAR EL DOT
    std::ofstream outfile ("grafo.dot");
    outfile << codigo << std::endl;
    outfile.close();

    //REMPLAZAR LA EXTENDION DE LA RUTA   
    //Basado en: https://www.oreilly.com/library/view/c-cookbook/0596007612/ch10s17.html             
    std::string::size_type pos = ruta.rfind('.', ruta.length());
    std::string png = "png";
    if (pos != std::string::npos) {
        ruta.replace(pos+1, png.length(), png);
    }

    //CREAR EL COMANDO DOT
    comando = "dot -Tpng grafo.dot -o";
    comando.append("'");
    comando.append(ruta);
    comando.append("'");

    //GENERAR EL GRAFO
    system(comando.c_str());
    std::cout << "MENSAJE: Reporte Block creado correctamente." << std::endl;
}

void journaling(std::vector<disco> &discos, int posDisco, int posParticion, std::string &ruta){
    //VARIABLES
    std::string codigo;                                      //Contenedor del codigo del dot
    disco &disc_uso = discos[posDisco];                      //Disco en uso
    montada &part_uso = disc_uso.particiones[posParticion]; //Particion Montada 
    FILE *archivo;                                          //Para leer el archivo
    MBR mbr;                                                //Para leer el mbr
    int posExtendida;                                       //Posicion para leer la extendida
    EBR ebr;                                                //Para leer los ebr de las particiones logicas          
    std::string comando;                                    //Instruccion a mandar a la consola para generar el comando
    int posInicio;                                          //Posicion donde inicia la particion
    sbloque sblock;                                         //Para leer el superbloque
    registro lregistro;                                     //Para leer el journal
    int contador = 0;                                       //Numero de registros leidos  
    char lectura;                                           //Para leer los caracteres del bitmap  
    int posLectura;                                         //Usado para las posiciones de lectura                         
    bool continuar = true;                                  //Para el ciclo de lectura

    //VERIFICAR QUE EXISTA EL ARCHIVO
    archivo = fopen(disc_uso.ruta.c_str(), "r+b");
    if(archivo == NULL){
        std::cout << "ERROR: No se encontro el disco." << std::endl;
        return;
    }

    //DETERMINAR LA POSICION DE INICIO PARA LEER LA PARTICION
    if(part_uso.posMBR != -1){
        MBR mbr;
        fseek(archivo, 0,SEEK_SET);
        fread(&mbr, sizeof(MBR), 1, archivo);
        posInicio = mbr.mbr_partition[part_uso.posMBR].part_start;
    }else{
        EBR ebr;
        fseek(archivo, part_uso.posEBR, SEEK_SET);
        fread(&ebr, sizeof(EBR), 1, archivo);
        posInicio = ebr.part_start;
    }

    //LEER EL SUPERBLOQUE
    fseek(archivo, posInicio, SEEK_SET);
    fread(&sblock, sizeof(sbloque), 1, archivo);

    //DETERMINAR SI ES EXT3
    if(sblock.s_filesystem_type != 3){
        std::cout << "ERROR: EL sistema de archivos no es EXT3." << std::endl;
        fclose(archivo);
        return;
    }

    //ESCRIBIR EL DOT
    codigo = "digraph G { \n rankdir = TB; node[shape = plaintext]; \n";

    //Declarar los nodos
    int posJournal = posInicio + sizeof(sbloque);
    while(continuar){
        posLectura = posJournal + (sizeof(registro) * contador);
        fseek(archivo, posLectura, SEEK_SET);
        fread(&lregistro, sizeof(registro), 1, archivo);

        if(lregistro.comando[0] == '\0'){
            continuar = false;
            continue;
        }

        std::string nombre = "LOG";
        nombre.append(std::to_string(contador + 1));
        codigo.append(nombre);

        nombre = "Log ";
        nombre.append(std::to_string(contador + 1));
        codigo.append("[ label = <<TABLE BORDER='2' CELLBORDER='1' CELLSPACING='0' BGCOLOR='#226f54'>\n");
        codigo.append("<TR><TD colspan ='5' ><b>");
        codigo.append(nombre);
        codigo.append("</b></TD></TR>\n");
        codigo.append("<TR><TD><b>Comando</b></TD><TD><b>Ruta</b></TD><TD><b>Nombre</b></TD><TD><b>Contenido</b></TD><TD><b>Fecha</b></TD></TR>"); 

        codigo.append("<TR>");
        codigo.append("<TD>");
        codigo.append(lregistro.comando);
        codigo.append("</TD>");
        codigo.append("<TD>");
        codigo.append(lregistro.path);
        codigo.append("</TD>");
        codigo.append("<TD>");
        codigo.append(lregistro.nombre);
        codigo.append("</TD>");
        codigo.append("<TD>");
        codigo.append(lregistro.contenido);
        codigo.append("</TD>");
        codigo.append("<TD>");
        codigo.append(asctime(gmtime(&lregistro.fecha)));
        codigo.append("</TD>");
        codigo.append("</TR>");
        codigo.append("</TABLE>>];\n");

        contador += 1;
    }

    //Unir los nodos
    for(int i = 0; i < contador; i++){
        std::string nombre = "LOG";
        nombre.append(std::to_string(i + 1)); 
        codigo.append(nombre);

        if(i != (contador - 1)){
            codigo.append("->");
        }
    }
    codigo.append("[color = white]");
    codigo.append("}");
    fclose(archivo);
    
    //GENERAR EL DOT
    std::ofstream outfile ("grafo.dot");
    outfile << codigo << std::endl;
    outfile.close();

    //REMPLAZAR LA EXTENDION DE LA RUTA   
    //Basado en: https://www.oreilly.com/library/view/c-cookbook/0596007612/ch10s17.html             
    std::string::size_type pos = ruta.rfind('.', ruta.length());
    std::string png = "png";
    if (pos != std::string::npos) {
        ruta.replace(pos+1, png.length(), png);
    }

    //CREAR EL COMANDO DOT
    comando = "dot -Tpng grafo.dot -o";
    comando.append("'");
    comando.append(ruta);
    comando.append("'");

    //GENERAR EL GRAFO
    system(comando.c_str());
    std::cout << "MENSAJE: Reporte Journaling creado correctamente." << std::endl;
}

void bm_inode(std::vector<disco> &discos, int posDisco, int posParticion, std::string &ruta){
    //VARIABLES
    std::string codigo;                                      //Contenedor del txt
    disco &disc_uso = discos[posDisco];                      //Disco en uso
    montada &part_uso = disc_uso.particiones[posParticion]; //Particion Montada 
    FILE *archivo;                                          //Para leer el archivo
    MBR mbr;                                                //Para leer el mbr
    int posExtendida;                                       //Posicion para leer la extendida
    EBR ebr;                                                //Para leer los ebr de las particiones logicas          
    std::string comando;                                    //Instruccion a mandar a la consola para generar el comando
    int posInicio;                                          //Posicion donde inicia la particion
    sbloque sblock;                                         //Para leer el superbloque
    inodo linodo;                                           //Para leer los inodos 
    char lectura;                                           //Para leer los caracteres del bitmap  
    int posLectura;                                         //Usado para las posiciones de lectura                         
    int contador = 0;                                       //Para el manejo de los espacios

    //VERIFICAR QUE EXISTA EL ARCHIVO
    archivo = fopen(disc_uso.ruta.c_str(), "r+b");
    if(archivo == NULL){
        std::cout << "ERROR: No se encontro el disco." << std::endl;
        return;
    }

    //DETERMINAR LA POSICION DE INICIO PARA LEER LA PARTICION
    if(part_uso.posMBR != -1){
        MBR mbr;
        fseek(archivo, 0,SEEK_SET);
        fread(&mbr, sizeof(MBR), 1, archivo);
        posInicio = mbr.mbr_partition[part_uso.posMBR].part_start;
    }else{
        EBR ebr;
        fseek(archivo, part_uso.posEBR, SEEK_SET);
        fread(&ebr, sizeof(EBR), 1, archivo);
        posInicio = ebr.part_start;
    }

    //LEER EL SUPERBLOQUE
    fseek(archivo, posInicio, SEEK_SET);
    fread(&sblock, sizeof(sbloque), 1, archivo);
    
    //ESCRIBIR EL TXT
    codigo = "";

    for(int i = 0; i < sblock.s_inodes_count; i++){
        posLectura = sblock.s_bm_inode_start + (sizeof(char) * i);
        fseek(archivo, posLectura, SEEK_SET);
        fread(&lectura, sizeof(lectura), 1, archivo);

        if(lectura == '1'){
            codigo.append("1");
        }else{
            codigo.append("0");
        }
        codigo.append(" ");

        contador += 1;
        if(contador == 20){
            codigo.append("\n");
            contador = 0;
        }
    }
    fclose(archivo);
    //GENERAR EL TXT
    std::ofstream outfile (ruta);
    outfile << codigo << std::endl;
    outfile.close();

    std::cout << "MENSAJE: Reporte bm_Inode creado correctamente." << std::endl;
}

void bm_block(std::vector<disco> &discos, int posDisco, int posParticion, std::string &ruta){
    //VARIABLES
    std::string codigo;                                      //Contenedor del txt
    disco &disc_uso = discos[posDisco];                      //Disco en uso
    montada &part_uso = disc_uso.particiones[posParticion]; //Particion Montada 
    FILE *archivo;                                          //Para leer el archivo
    MBR mbr;                                                //Para leer el mbr
    int posExtendida;                                       //Posicion para leer la extendida
    EBR ebr;                                                //Para leer los ebr de las particiones logicas          
    std::string comando;                                    //Instruccion a mandar a la consola para generar el comando
    int posInicio;                                          //Posicion donde inicia la particion
    sbloque sblock;                                         //Para leer el superbloque
    bcarpetas lcarpeta;                                     //Para leer bloques de carpetas
    barchivos larchivo;                                     //Para leer bloques de archivos
    bapuntadores lapuntador;                                //Para leer bloques de apuntadores  
    char lectura;                                           //Para leer los caracteres del bitmap  
    int posLectura;                                         //Usado para las posiciones de lectura                         
    int contador = 0;                                       //Para los saltos de linea

    //VERIFICAR QUE EXISTA EL ARCHIVO
    archivo = fopen(disc_uso.ruta.c_str(), "r+b");
    if(archivo == NULL){
        std::cout << "ERROR: No se encontro el disco." << std::endl;
        return;
    }

    //DETERMINAR LA POSICION DE INICIO PARA LEER LA PARTICION
    if(part_uso.posMBR != -1){
        MBR mbr;
        fseek(archivo, 0,SEEK_SET);
        fread(&mbr, sizeof(MBR), 1, archivo);
        posInicio = mbr.mbr_partition[part_uso.posMBR].part_start;
    }else{
        EBR ebr;
        fseek(archivo, part_uso.posEBR, SEEK_SET);
        fread(&ebr, sizeof(EBR), 1, archivo);
        posInicio = ebr.part_start;
    }

    //LEER EL SUPERBLOQUE
    fseek(archivo, posInicio, SEEK_SET);
    fread(&sblock, sizeof(sbloque), 1, archivo);
    
    //ESCRIBIR EL TXT
    codigo = "";

    for(int i = 0; i < sblock.s_blocks_count; i++){
        posLectura = sblock.s_bm_block_start + (sizeof(char) * i);
        fseek(archivo, posLectura, SEEK_SET);
        fread(&lectura, sizeof(lectura), 1, archivo);

        if(lectura == 'c' || lectura == 'p' || lectura == 'a'){
            codigo.append("1");
        }else{
            codigo.append("0");
        }
        codigo.append(" ");

        contador += 1;
        if(contador == 20){
            codigo.append("\n");
            contador = 0;
        }
    }
    fclose(archivo);

    //GENERAR EL TXT
    std::ofstream outfile (ruta);
    outfile << codigo << std::endl;
    outfile.close();

    
    std::cout << "MENSAJE: Reporte bm_Block creado correctamente." << std::endl;
}

