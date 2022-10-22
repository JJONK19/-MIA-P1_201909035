#include <iostream>
#include <string>

#include "analizador.h"

using namespace std;

int main(){
    bool continuar = true;
    string comando = "";

    //Lectura de comandos
    cout << "PROYECTO 1 - ARCHIVOS - 201909035" << endl;
    cout << "------------------------------------------------------------------------" << endl;
    while (continuar){
        getline(cin, comando);

        //Salir de la aplicación
        if(comando == "EXIT" || comando == "exit"){
            continuar = false;
            continue;
        }

        //Ejecutar Instruccion
        ejecutar(comando);
        comando.clear();                            
    }

    return 0;
}