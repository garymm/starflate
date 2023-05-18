#include <expected>

auto main() -> int
{
  const auto x = std::expected<int, double>{};
  (void)x;
}
