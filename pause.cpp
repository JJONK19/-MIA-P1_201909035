#include "pause.h"

void pause(){
    std::string confirmar;
    std::cout << "Pulse Intro para continuar:" << std::endl;
    std::getline(std::cin, confirmar);
}