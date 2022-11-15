#include <iostream>
#include <string>

#include "analizador.h"
#include "structs.h"

using namespace std;
vector<disco> discos;
usuario sesion;

int main(){
    //VARIABLES
    bool continuar = true;
    string comando = "";  

    //CONSOLA DE COMANDOS
    cout << "****************************************************************************************************" << endl;
    cout << endl;
    cout << "PROYECTO 1 ARCHIVOS - 201909035" << endl;
    cout << endl;
    cout << "****************************************************************************************************" << endl;
    while (continuar){
        cout << "----------------------------------------------------------------------------------------------------" << endl;
        cout << "INSTRUCCION:" <<endl; 
        getline(cin, comando);
        cout << endl;

        //Salir de la aplicación
        if(comando == "EXIT" || comando == "exit"){
            continuar = false;
            cout << endl;
            cout << "----------------------------------------------------------------------------------------------------" << endl;          
            continue;
        }

        //Ejecutar Instruccion
        cout << "EJECUCIÓN:" <<endl;
        ejecutar(comando, sesion, discos);
        comando.clear();   
        cout << endl;
    }

    return 0;
}