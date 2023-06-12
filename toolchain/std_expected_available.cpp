#include <expected>

auto main() -> int
{
  [[maybe_unused]] const auto x = std::expected<int, double>{};
}
