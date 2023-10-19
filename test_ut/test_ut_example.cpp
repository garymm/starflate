#include <boost/ut.hpp>

#include <array>
#include <ranges>

auto main() -> int
{
  using ::boost::ut::expect;
  using ::boost::ut::range_eq;
  using ::boost::ut::test;

  test("test range eq, lhs smaller") = [] {
    constexpr auto data1 = std::array{1, 2, 3};
    constexpr auto data2 = std::views::iota(1, 5);

    expect(range_eq(data1, data2));
  };

  test("test range eq, lhs bigger") = [] {
    constexpr auto data1 = std::array{1, 2, 3};
    constexpr auto data2 = std::views::iota(1, 3);

    expect(range_eq(data1, data2));
  };

  test("test range eq, value mismatch") = [] {
    constexpr auto data1 = std::array{1, 2, 4};
    constexpr auto data2 = std::views::iota(1, 4);

    expect(range_eq(data1, data2));
  };
}
