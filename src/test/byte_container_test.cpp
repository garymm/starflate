#include "src/test/byte_container.hpp"

#include "boost/ut.hpp"

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <fstream>
#include <ios>
#include <iostream>
#include <ranges>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

namespace test {

template <class FsPath, std::ranges::contiguous_range Range>
  requires std::same_as<std::byte, std::ranges::range_value_t<Range>>
auto write_file(FsPath&& filename, const Range& data) -> void
{
  auto out = std::ofstream{
      std::forward<FsPath>(filename), std::ios::binary | std::ios::trunc};

  std::copy(
      reinterpret_cast<const char*>(std::ranges::cbegin(data)),
      reinterpret_cast<const char*>(std::ranges::cend(data)),
      std::ostreambuf_iterator<char>{out});
}

}  // namespace test

auto main() -> int
{
  using ::boost::ut::eq;
  using ::boost::ut::expect;
  using ::boost::ut::range_eq;
  using ::boost::ut::test;
  using ::boost::ut::operators::operator|;

  static constexpr auto data =
      std::array{std::byte{0}, std::byte{1}, std::byte{2}, std::byte{3}};

  test("write then read file") = [] {
    constexpr auto filename = "testfile1";

    ::test::write_file(filename, data);

    auto in = std::ifstream{filename, std::ios::binary};
    auto inbuf = std::array<std::byte, 4>{};

    std::copy(
        std::istreambuf_iterator<char>{in},
        std::istreambuf_iterator<char>{},
        reinterpret_cast<char*>(inbuf.begin()));

    expect(eq(inbuf, data));
  };

  test("write then read file") = [] {
    constexpr auto filename = "testfile2";

    ::test::write_file(filename, data);

    const auto bytes = starflate::test::byte_vector{filename};

    expect(range_eq(bytes, data));
  };

  {
    static constexpr auto str = "abcd";

    test("construct from string-like") = [](auto s) {
      constexpr auto data1 = std::array{
          static_cast<std::byte>('a'),
          static_cast<std::byte>('b'),
          static_cast<std::byte>('c'),
          static_cast<std::byte>('d'),
      };
      const auto data2 = starflate::test::byte_vector{std::in_place, s};

      expect(range_eq(data1, data2));
    } | std::tuple{str, std::string{str}, std::string_view{str}};
  }
}
