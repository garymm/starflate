#include <boost/ut.hpp>

#include <array>
#include <cstddef>
#include <ranges>
#include <sstream>

auto main() -> int
{
  using ::boost::ut::eq;
  using ::boost::ut::expect;
  using ::boost::ut::range_eq;
  using ::boost::ut::test;

  // all of these tests are intended to fail in order to demonstrate information
  // printed on test failure

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

  test("print byte") = [] {
    constexpr auto expected = "01100000";

    const auto actual = (std::stringstream{} << std::byte{0b1110'0000}).str();

    expect(eq(expected, actual));
  };

  test("print byte array") = [] {
    constexpr auto data1 = std::array{std::byte{0xDEU}, std::byte{0xADU}};
    constexpr auto data2 = std::array{std::byte{0xBEU}, std::byte{0xEFU}};

    expect(eq(data1, data2));
  };
}
