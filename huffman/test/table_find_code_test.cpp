#include "huffman/huffman.hpp"

#include <boost/ut.hpp>

#include <algorithm>
#include <string_view>

auto main() -> int
{
  using ::boost::ut::expect;
  using ::boost::ut::test;

  namespace huffman = ::starflate::huffman;
  using namespace huffman::literals;

  static constexpr auto table1 =  // clang-format off
    huffman::table{
    huffman::table_contents,
    {std::pair{0_c,     'e'},
              {10_c,    'i'},
              {110_c,   'n'},
              {1110_c,  'q'},
              {11110_c, '\4'},
              {11111_c, 'x'}}
  };
  // clang-format on

  test("finds code in table") = [] {
    static_assert('e' == table1.find(0_c).value()->symbol);
    static_assert('i' == table1.find(10_c).value()->symbol);
    static_assert('n' == table1.find(110_c).value()->symbol);
    static_assert('q' == table1.find(1110_c).value()->symbol);
    static_assert('\4' == table1.find(11110_c).value()->symbol);
    static_assert('x' == table1.find(11111_c).value()->symbol);
  };

  // bitsize values we compare against are derived from the code
  // NOLINTBEGIN(readability-magic-numbers)

  test("code not in table but valid prefix") = [] {
    static_assert(table1.find(1_c).error()->symbol == 'i');
    static_assert(table1.find(1_c).error()->bitsize() == 2);

    static_assert(table1.find(11_c).error()->symbol == 'n');
    static_assert(table1.find(11_c).error()->bitsize() == 3);

    static_assert(table1.find(111_c).error()->symbol == 'q');
    static_assert(table1.find(111_c).error()->bitsize() == 4);

    static_assert(table1.find(1111_c).error()->symbol == '\4');
    static_assert(table1.find(1111_c).error()->bitsize() == 5);
  };

  test("code not in table but valid prefix, using explicit pos") = [] {
    constexpr auto pos1 = table1.find(1_c).error();
    static_assert(pos1->symbol == 'i');
    static_assert(pos1->bitsize() == 2);

    constexpr auto pos2 = table1.find(11_c, pos1).error();
    static_assert(pos2->symbol == 'n');
    static_assert(pos2->bitsize() == 3);

    constexpr auto pos3 = table1.find(111_c, pos2).error();
    static_assert(pos3->symbol == 'q');
    static_assert(pos3->bitsize() == 4);

    static_assert(table1.find(1111_c, pos3).error()->symbol == '\4');
    static_assert(table1.find(1111_c, pos3).error()->bitsize() == 5);
  };

  // NOLINTEND(readability-magic-numbers)

  test("code bitsize exceeds all codes in table") = [] {
    static_assert(table1.find(000000_c).error() == table1.end());
    static_assert(table1.find(111111_c).error() == table1.end());
  };

  test("code is smaller than any code in table") = [] {
    static constexpr auto table =  // clang-format off
      huffman::table{
        huffman::table_contents,
        {std::pair{00_c,     'e'},
                  {110_c,    'i'},
                  {1110_c,   'n'},
                  {11110_c,  'q'},
                  {111110_c, '\4'},
                  {111111_c, 'x'}}};
    // clang-format on

    static_assert(table.find(0_c).error() == table.begin());
    static_assert(table.find(1_c).error() == table.begin());
  };
}
