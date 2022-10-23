#ifndef STRUCTS_H
#define STRUCTS_H

#include <time.h>
#include <string>
#include <vector>
#include <regex>

//********** PARTICION ********** 
struct particion{
    char part_status;                       //0 para desactivada, 1 para activa
    char part_type = 'p';                   //Partición Primaria o Extendida (p ó e)
    char part_fit = 'w';                    //Fit: Best(b), First(f), Worst(w)
    int part_start;                         //Posicion inicial de la la partición
    int part_s;                             //Tamaño de la partición(bytes)
    char part_name[16];                     //Nombre de la partición
};

//********** MBR **********
struct MBR{
    int mbr_tamano;                         //Tamaño en bytes del disco
    time_t mbr_fecha_creacion;              //Fecha y Hora 
    int mbr_dsk_signature;                  //Int random que identifica el disco
    char dsk_fit;                           //Fit: Best(b), First(f), Worst(w)
    particion mbr_partition[4];             //Array de particiones 
};

//********** EBR **********
struct EBR{
    char part_status;                       //0 para desactivada, 1 para activa
    char part_fit = 'w';                    //Fit: Best(b), First(f), Worst(w)
    int part_start;                         //Posicion inicial de la la partición
    int part_s;                             //Tamaño de la partición(bytes)
    int part_next;                          //Es -1  si no hay otro EBR
    char part_name[16];                     //Nombre de la partición logica

};

//********** POSICIONES DE LAS PARTICIONES **********
//Indica un resumen de donde estan las particiones para evitar tener que leer constantemente
//el disco. Se usa para encontrar los espacios vacíos.

struct posicion{
    int inicio;                             
    int fin;
    char tipo;
    std::string nombre;
    int tamaño; 

    bool operator<(const posicion& a) const
    {
        return inicio < a.inicio;
    }
};

//********** ESPACIOS VACIOS **********
//Indica el inicio y fin de los espacios vacíos (particiones primarias y extendida)
struct libre{
    int inicio;
    int tamaño;

    bool operator<(const libre& a) const
    {
        return tamaño < a.tamaño;
    }
};

//Indica el espacio vacío dentro de la partición expandida
struct libreL{
    int inicioEBR;              //Para leer el EBR de la partición
    int finLogica;              //Donde termina la particion logica
    int tamaño;                 //Espacio Libre
    bool cabecera;              //Para diferenciar si es la cabecera de la extendida

    bool operator<(const libreL& a) const
    {
        return tamaño < a.tamaño;
    }
};

//********** MONTAR DISCOS **********
//Maneja la posición de la partición dentro del disco
struct montada{
    std::string id;                             //ID de a particion montada: 035Disco#
    int posEBR = -1;                            //Si es logica es diferente a -1
    int posMBR = -1;                            //Si no es logica es diferente a -1
    std::string nombre;                         //Nombre de la particion
    int tamaño;                                 //Tamaño de la particion en bytes
};

//Maneja los datos del disco. Necesario para leer las particiones. 
struct disco{
    std::string ruta;                           //Ruta del disco
    std::string nombre;                         //Nombre del disco
    int contador = 1;                           //Sirve para el ID de la montada
    std::vector<montada> particiones;           //Particiones del disco montadas
};

//********** SUPER BLOQUE **********
struct sbloque{
    int s_filesystem_type;              //3 para ext3 y 2 para ext2
    int s_inodes_count;                 //Total de inodos
    int s_blocks_count;                 //Total de bloques
    int s_free_blocks_count;            //Bloques libres
    int s_free_inodes_count;            //Inodos libres
    time_t s_mtime;                     //Ultima fecha montado
    time_t s_umtime;                    //Ultima fecha desmontado
    int s_mnt_count;                    //Numero de veces montado
    int s_magic;                        //0xEF53 - Default siempre
    int s_inode_s;                      //Tamaño del inodo
    int s_block_s;                      //Tamaño del bloque
    int s_firts_ino;                    //Posicion del primer inodo libre
    int s_first_blo;                    //Poisicion del primer bloque libre
    int s_bm_inode_start;               //Posicion del bitmap inodos
    int s_bm_block_start;               //Posicion bitmap bloques
    int s_inode_start;                  //Inicio de los inodos
    int s_block_start;                  //Inicio de los bloques
};

//********** INODOS **********
struct inodo{
    int i_uid;                          //ID del dueño                          
    int i_gid;                          //ID del grupo 
    int i_s;                            //Tamaño 
    time_t i_atime;                     //Ultima fecha de lectura sin modificar
    time_t i_ctime;                     //Fecha de creación
    time_t i_mtime;                     //Ultima fecha de modificación
    int i_block[15];                    //-1: Sin usar 0-12: Directos 13-15: Indirectos
    char i_type;                        //1: Archivo 0: Carpeta
    int i_perm;                         //Permisos
};

//********** BLOQUES **********
struct content{
    char b_name[12];                    //Nombre de la carpeta o archivo
    int b_inodo;                        //Numero de inodo que contiene la carpeta o archivo
};

struct bcarpetas{
    content b_content[4];                 
};

struct barchivos{
    char b_content[64];         
};

struct bapuntadores{
    int b_pointers[16];                 //Guarda la posicion de otro bloque (apuntador, bloque o carpeta)
};

//********** JOURNALING **********
struct registro{
    char comando[10];                   //Comando utilizado(mkdir, rmdir, etc)                
    char path[40];                      //Ruta de la operación
    char nombre[20];                    //Nombre del archivo o carpeta
    char contenido[100];                //Para archivos, almacena el contenido
    time_t fecha;                       //Fecha de la accion realizada
};

//********** LOGIN **********
struct usuario{
    std::string user;                   //ID del usuario
    std::string pass;                   //Contraseña del usuario
    std::string disco;                  //ID de la particion en la que esta trabajando
    std::string grupo;                  //Grupo al que pertenece el usuario
};

//********** CONSTANTES **********
const int EndMBR = sizeof(MBR) + 1;     //Posicion de bytes del mbr para comenzar a escribir
const std::regex flecha("->");

#endif

