#include <execution>
#include <iostream>
#include <numeric>
#include <ranges>

auto main() -> int
{
  constexpr auto iv = std::views::iota(1, 10);

  std::cout
      << "parallel transform reduce: "
      << std::transform_reduce(
             std::execution::par, iv.begin(), iv.end(), iv.begin(), 0)
      << '\n';
}
