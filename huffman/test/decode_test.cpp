#include "huffman/huffman.hpp"

#include <boost/ut.hpp>

#include <array>
#include <climits>
#include <cstddef>
#include <utility>
#include <vector>

auto main() -> int
{
  using ::boost::ut::expect;
  using ::boost::ut::test;

  namespace huffman = ::gpu_deflate::huffman;
  using namespace huffman::literals;

  test("basic") = [] {
    // encoded data from dahuffman readme.rst, but in hex.
    constexpr std::array<std::byte, 6> encoded_bytes = {
        std::byte{0x86},
        std::byte{0x7c},
        std::byte{0x25},
        std::byte{0x13},
        std::byte{0x69},
        std::byte{0x40}};

    constexpr char eot = {'\4'};
    static constexpr auto code_table =  // clang-format off
      huffman::table{
        huffman::table_contents,
        {std::pair{00000_c, eot},
                  {00001_c, 'x'},
                  {0001_c,  'q'},
                  {001_c,   'n'},
                  {01_c,    'i'},
                  {1_c,     'e'}}
      };  // clang-format on

    std::vector<char> output_buf;
    auto result = decode(
        code_table,
        huffman::bit_span{
            encoded_bytes.data(), encoded_bytes.size() * CHAR_BIT},
        std::back_inserter(output_buf));

    const std::vector<char> expected = {
        'e', 'x', 'e', 'n', 'e', 'e', 'e', 'e', 'x', 'n',
        'i', 'q', 'n', 'e', 'i', 'e', 'i', 'n', 'i', eot,
    };
    expect(output_buf == expected);

    // result should point to the back of output_buf.
    *result = '1';
    expect(output_buf[output_buf.size() - 1] == '1');
  };
}
