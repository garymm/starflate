#include <iostream>
#include "hip_info.hpp"

auto main() -> int
{
    std::cout << '\n';

    printCompilerInfo();

    printDeviceProps();

    std::cout << '\n';
}
