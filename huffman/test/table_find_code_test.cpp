#include "huffman/huffman.hpp"

#include <boost/ut.hpp>

#include <algorithm>
#include <string_view>

auto main() -> int
{
  using ::boost::ut::expect;
  using ::boost::ut::test;

  namespace huffman = ::gpu_deflate::huffman;
  using namespace huffman::literals;

  static constexpr auto table1 =  // clang-format off
    huffman::table{
    huffman::table_contents,
    {std::pair{00000_c, '\4'},
              {00001_c, 'x'},
              {0001_c,  'q'},
              {001_c,   'n'},
              {01_c,    'i'},
              {1_c,     'e'}}
  };
  // clang-format on

  test("finds code in table") = [] {
    static_assert('e' == table1.find(1_c).value()->symbol);
    static_assert('i' == table1.find(01_c).value()->symbol);
    static_assert('n' == table1.find(001_c).value()->symbol);
    static_assert('q' == table1.find(0001_c).value()->symbol);
    static_assert('x' == table1.find(00001_c).value()->symbol);
    static_assert('\4' == table1.find(00000_c).value()->symbol);
  };

  // bitsize values we compare against are derived from the code
  // NOLINTBEGIN(readability-magic-numbers)

  test("code not in table but valid prefix") = [] {
    static_assert(table1.find(0_c).error()->symbol == 'i');
    static_assert(table1.find(0_c).error()->bitsize() == 2);

    static_assert(table1.find(00_c).error()->symbol == 'n');
    static_assert(table1.find(00_c).error()->bitsize() == 3);

    static_assert(table1.find(000_c).error()->symbol == 'q');
    static_assert(table1.find(000_c).error()->bitsize() == 4);

    // ordering of elements with the same bitsize is unspecified
    static_assert(table1.find(0000_c).error()->bitsize() == 5);
  };

  test("code not in table but valid prefix, using explicit pos") = [] {
    constexpr auto pos1 = table1.find(0_c).error();
    static_assert(pos1->symbol == 'i');
    static_assert(pos1->bitsize() == 2);

    constexpr auto pos2 = table1.find(00_c, pos1).error();
    static_assert(pos2->symbol == 'n');
    static_assert(pos2->bitsize() == 3);

    constexpr auto pos3 = table1.find(000_c, pos2).error();
    static_assert(pos3->symbol == 'q');
    static_assert(pos3->bitsize() == 4);

    // ordering of elements with the same bitsize is unspecified
    static_assert(table1.find(0000_c, pos3).error()->bitsize() == 5);
  };

  // NOLINTEND(readability-magic-numbers)

  test("code bitsize exceeds all codes in table") = [] {
    static_assert(table1.find(000000_c).error() == table1.end());
  };

  test("code is smaller than any code in table") = [] {
    static constexpr auto table =  // clang-format off
      huffman::table{
      huffman::table_contents,
      {std::pair{00000_c, '\4'},
       {00001_c, 'x'},
       {0001_c,  'q'},
       {001_c,   'n'},
       {01_c,    'i'},
       {11_c,     'e'}}
    };
    // clang-format on

    static_assert(table.find(1_c).error() == table.begin());
  };
}
