#include <ref_vk.h>
#include <iostream>
#include <windows.h>


int main(int argc, char *argv[]) {

    std::cout << R_Init() << "\n";
    R_Shutdown();
    std::cout << R_GetConfigName() << "\n";

    return 0;
}
