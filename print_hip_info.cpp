#include <iostream>
#include "hip_info.hpp"
int main(int argc, char* argv[]) {
    std::cout << std::endl;

    printCompilerInfo();

    printDeviceProps();

    std::cout << std::endl;
}
