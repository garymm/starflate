#include "huffman/huffman.hpp"

#include <boost/ut.hpp>

#include <algorithm>
#include <cstdint>
#include <ranges>
#include <utility>
#include <vector>

auto main() -> int
{
  using ::boost::ut::expect;
  using ::boost::ut::test;

  namespace huffman = ::starflate::huffman;
  using namespace huffman::literals;

  test("table with DEFLATE canonical code, RFC example 1") = [] {
    // https://datatracker.ietf.org/doc/html/rfc1951#page-9
    static constexpr auto actual =  // clang-format off
      huffman::table{
        huffman::symbol_bitsize,
        {std::pair{'A', 2},
                  {'B', 1},
                  {'C', 3},
                  {'D', 3}}};
    // clang-format on

    static constexpr auto expected =  // clang-format off
      huffman::table{
        huffman::table_contents,
        {std::pair{0_c,   'B'},
                  {10_c,  'A'},
                  {110_c, 'C'},
                  {111_c, 'D'}}};
    // clang-format on

    static_assert(std::ranges::equal(actual, expected));

    static constexpr auto actual2 = huffman::table<char, 4>{
        huffman::symbol_bitsize,
        {{'A', 2},  //
         {'B', 1},
         {{'C', 'D'}, 3}}};

    static_assert(std::ranges::equal(actual2, expected));
  };

  test("table with DEFLATE canonical code, RFC example 2") = [] {
    // https://datatracker.ietf.org/doc/html/rfc1951#page-9
    const auto actual =  // clang-format off
      huffman::table{
        huffman::symbol_bitsize,
        std::vector<std::pair<huffman::symbol_span<char>, std::uint8_t>>{
          {'A', 3},
          {'B', 3},
          {'C', 3},
          {'D', 3},
          {'E', 3},
          {'F', 2},
          {'G', 4},
          {'H', 4}}};
    // clang-format on

    static constexpr auto expected =  // clang-format off
      huffman::table{
        huffman::table_contents,
        {std::pair{00_c,    'F'},
                  {010_c,   'A'},
                  {011_c,   'B'},
                  {100_c,   'C'},
                  {101_c,   'D'},
                  {110_c,   'E'},
                  {1110_c,  'G'},
                  {1111_c,  'H'}}};
    // clang-format on

    expect(std::ranges::equal(actual, expected));

    const auto actual2 = huffman::table{
        huffman::symbol_bitsize,
        std::vector<std::pair<huffman::symbol_span<char>, std::uint8_t>>{
            {'F', 2},         //
            {{'A', 'E'}, 3},  //
            {{'G', 'H'}, 4}}};

    expect(std::ranges::equal(actual2, expected));
  };

  test("table with DEFLATE canonical code, static literal/length") = [] {
    // NOLINTBEGIN(readability-magic-numbers)

    // static literal/length table
    //
    // literal/length  bitsize  code
    // ==============  =======  =========================
    //   0 - 143       8          0011'0000 - 1011'1111
    // 144 - 255       9        1'1001'0000 - 1'1111'1111
    // 256 - 279       7           000'0000 - 001'0111
    // 280 - 287       8          1100'0000 - 1100'0111

    static constexpr auto table =  // clang-format off
      huffman::table<std::uint16_t, 288>{
        huffman::symbol_bitsize,
        {{{  0, 143}, 8},
         {{144, 255}, 9},
         {{256, 279}, 7},
         {{280, 287}, 8}}};
                                   // clang-format on

    auto start_code = std::uint16_t{0b0011'0000};
    for (auto code = start_code; code <= std::uint16_t{0b1011'1111}; ++code) {
      const auto expected_symbol = code - start_code;

      const auto result = table.find({8, code});

      expect(result and expected_symbol == (**result).symbol);
    }

    start_code = std::uint16_t{0b1'1001'0000};
    for (auto code = start_code; code <= std::uint16_t{0b1'1111'1111}; ++code) {
      const auto expected_symbol = code - start_code + 144;

      const auto result = table.find({9, code});

      expect(result and expected_symbol == (**result).symbol);
    }

    start_code = std::uint16_t{0b000'0000};
    for (auto code = start_code; code <= std::uint16_t{0b001'0111}; ++code) {
      const auto expected_symbol = code - start_code + 256;

      const auto result = table.find({7, code});

      expect(result and expected_symbol == (**result).symbol);
    }

    start_code = std::uint16_t{0b1100'0000};
    for (auto code = start_code; code <= std::uint16_t{0b01100'0111}; ++code) {
      const auto expected_symbol = code - start_code + 280;

      const auto result = table.find({8, code});

      expect(result and expected_symbol == (**result).symbol);
    }

    // NOLINTEND(readability-magic-numbers)
  };
}
