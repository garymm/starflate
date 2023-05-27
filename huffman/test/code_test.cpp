#include "huffman/huffman.hpp"

#include <boost/ut.hpp>

#include <algorithm>
#include <string_view>

auto main() -> int
{
  using namespace std::literals::string_view_literals;
  using ::boost::ut::expect;
  using ::boost::ut::test;

  namespace huffman = ::gpu_deflate::huffman;

  static const auto to_string = [](huffman::code code) {
    auto ss = std::stringstream{};
    ss << code;
    return ss.str();
  };

  // NOLINTBEGIN(readability-magic-numbers)

  test("code is streamable") = [] {
    expect(std::ranges::equal(
        "0"sv, to_string(huffman::code{.bitsize = 1, .value = 0})));
    expect(std::ranges::equal(
        "10"sv, to_string(huffman::code{.bitsize = 2, .value = 0b10})));
    expect(std::ranges::equal(
        "110"sv, to_string(huffman::code{.bitsize = 3, .value = 0b110})));
    expect(std::ranges::equal(
        "1110"sv, to_string(huffman::code{.bitsize = 4, .value = 0b1110})));
    expect(std::ranges::equal(
        "11110"sv, to_string(huffman::code{.bitsize = 5, .value = 0b11110})));
    expect(std::ranges::equal(
        "11111"sv, to_string(huffman::code{.bitsize = 5, .value = 0b11111})));
  };

  static const auto has_bits = [](std::string_view sv, huffman::code code) {
    return std::ranges::equal(sv, code.bit_view());
  };

  test("code is convertible to range") = [] {
    static_assert(has_bits("0"sv, huffman::code{.bitsize = 1, .value = 0}));
    static_assert(
        has_bits("11110"sv, huffman::code{.bitsize = 5, .value = 0b11110}));
  };

  test("code is constructible with literal") = [] {
    using namespace huffman::literals;

    expect(has_bits("1"sv, 1_c));
    expect(has_bits("0"sv, 0_c));

    expect(has_bits("11"sv, 11_c));
    expect(has_bits("10"sv, 10_c));
    expect(has_bits("01"sv, 01_c));
    expect(has_bits("00"sv, 00_c));

    expect(has_bits("11111"sv, 11111_c));
    expect(has_bits("11110"sv, 11110_c));
    expect(has_bits("00000"sv, 00000_c));
  };

  // NOLINTEND(readability-magic-numbers)
}
