#include "huffman/huffman.hpp"

#include <boost/ut.hpp>

#include <algorithm>
#include <cstdint>
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
  };

  test("table with DEFLATE canonical code, RFC example 2") = [] {
    // https://datatracker.ietf.org/doc/html/rfc1951#page-9
    const auto actual =  // clang-format off
      huffman::table{
        huffman::symbol_bitsize,
        std::vector<std::pair<char, std::uint8_t>>{{'A', 3},
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
  };
}
