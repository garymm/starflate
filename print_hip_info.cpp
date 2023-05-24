#include "hip_info.hpp"

#include <iostream>

auto main() -> int
{
  std::cout << '\n';

  printCompilerInfo();

  printDeviceProps();

  std::cout << '\n';
}
