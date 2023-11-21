#include "huffman/huffman.hpp"

#include <boost/ut.hpp>

#include <algorithm>
#include <tuple>
#include <utility>
#include <vector>

auto main() -> int
{
  using ::boost::ut::aborts;
  using ::boost::ut::expect;
  using ::boost::ut::test;

  namespace huffman = ::starflate::huffman;
  using namespace huffman::literals;

  test("code table constructible from code-symbol mapping") = [] {
    const auto t = huffman::table<char>{
        // clang-format off
        huffman::table_contents,
        {{0_c, 'e'},
         {10_c, 'i'},
         {110_c, 'n'},
         {1110_c, 'q'},
         {11110_c, '\4'},
         {11111_c, 'x'}}};
    // clang-format on

    auto ss = std::stringstream{};
    ss << t;

    constexpr auto table =
        "Bits\tCode\tValue\tSymbol\n"
        "1\t0\t0\t`e`\n"
        "2\t10\t2\t`i`\n"
        "3\t110\t6\t`n`\n"
        "4\t1110\t14\t`q`\n"
        "5\t11110\t30\t`\4`\n"
        "5\t11111\t31\t`x`\n";

    expect(table == ss.str()) << ss.str();
  };

  test("code table constructible from code-symbol mapping from different "
       "ranges") = [] {
    // `t1` deduces a C-array for the range
    const auto t1 =  // clang-format off
      huffman::table<char>{
        huffman::table_contents,
        {{0_c, 'e'},
         {10_c, 'i'},
         {110_c, 'n'},
         {1110_c, 'q'},
         {11110_c, '\4'},
         {11111_c, 'x'}}};
    // clang-format on

    const auto t2 =  // clang-format off
      huffman::table<char>{
        huffman::table_contents,
        std::vector{
            std::tuple{0_c,     'e'},
                      {10_c,    'i'},
                      {110_c,   'n'},
                      {1110_c,  'q'},
                      {11110_c, '\4'},
                      {11111_c, 'x'}}};
    // clang-format on

    expect(std::ranges::equal(t1, t2));
  };

  test("code table constructible from code-symbol mapping with deduction "
       "guides") = [] {
    const auto t1 =  // clang-format off
      huffman::table{
        huffman::table_contents,
        {std::pair{0_c,     'e'},
                  {10_c,    'i'},
                  {110_c,   'n'},
                  {1110_c,  'q'},
                  {11110_c, '\4'},
                  {11111_c, 'x'}}};
    // clang-format on

    const auto t2 =  // clang-format off
      huffman::table{
        huffman::table_contents,
        std::vector{
            std::tuple{0_c,     'e'},
                      {10_c,    'i'},
                      {110_c,   'n'},
                      {1110_c,  'q'},
                      {11110_c, '\4'},
                      {11111_c, 'x'}}};
      // clang-format off

    expect(std::ranges::equal(t1, t2));
  };

  test("code table constexpr constructible from code-symbol") = [] {
    static constexpr auto t1 = // clang-format off
      huffman::table{
        huffman::table_contents,
        {std::pair{0_c,     'e'},
                  {10_c,    'i'},
                  {110_c,   'n'},
                  {1110_c,  'q'},
                  {11110_c, '\4'},
                  {11111_c, 'x'}}};
    // clang-format on

    static constexpr auto t2 =  // clang-format off
        huffman::table<char, 6>{
          huffman::table_contents,
          {{0_c,     'e'},
           {10_c,    'i'},
           {110_c,   'n'},
           {1110_c,  'q'},
           {11110_c, '\4'},
           {11111_c, 'x'}}};
    // clang-format on

    expect(std::ranges::equal(t1, t2));
  };
}
