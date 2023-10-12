#include "huffman/huffman.hpp"

#include <boost/ut.hpp>

#include <algorithm>
#include <utility>

auto main() -> int
{
  using ::boost::ut::expect;
  using ::boost::ut::test;

  namespace huffman = ::starflate::huffman;
  using namespace huffman::literals;

  test("table with DEFLATE canonical code, example 1") = [] {
    static constexpr auto actual =  // clang-format off
      huffman::table{
        huffman::table_contents,
        {std::pair{00_c,  'A'},
                  {1_c,   'B'},
                  {011_c, 'C'},
                  {010_c, 'D'}}}.canonicalize();
    // clang-format on

    static constexpr auto expected =  // clang-format off
      huffman::table{
        huffman::table_contents,
        {std::pair{0_c,   'B'},
                  {10_c,  'A'},
                  {110_c, 'C'},
                  {111_c, 'D'}}};
    // clang-format on

    expect(std::ranges::equal(actual, expected));
  };

  test("table with DEFLATE canonical code, example 2") = [] {
    // NOTE: t1 is an *invalid* table (as initially specified) because
    // some codes are prefixes of others.
    static constexpr auto actual =  // clang-format off
      huffman::table{
        huffman::table_contents,
        {std::pair{000_c,   'A'},
                  {001_c,   'B'},
                  {010_c,   'C'},
                  {011_c,   'D'},
                  {100_c,   'E'},
                  {11_c,    'F'},
                  {0111_c,  'G'},
                  {1111_c,  'H'}}}.canonicalize();
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

  test("canonicalization is idempotent") = [] {
    static constexpr auto t1 =  // clang-format off
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

    auto t2 = t1;
    t2.canonicalize();

    expect(std::ranges::equal(t1, t2));
    expect(std::ranges::equal(t1, t2.canonicalize()));
  };

  test("canonicalize invocable on empty table") = [] {
    static constexpr auto actual = huffman::table<char, 0>{}.canonicalize();
    auto expected = huffman::table<char, 0>{};

    expect(std::ranges::equal(actual, expected)) << actual << '\n' << expected;
  };

  test("canonicalize invocable on single element table") = [] {
    static constexpr auto actual = huffman::table{
        huffman::table_contents,
        {std::pair{0_c, 'A'}}}.canonicalize();
    auto expected = huffman::table<char, 1>{std::array{'A'}, std::nullopt};

    expect(std::ranges::equal(actual, expected)) << actual << '\n' << expected;
  };
}
