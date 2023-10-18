#include "huffman/huffman.hpp"

#include <boost/ut.hpp>

#include <array>
#include <cstddef>
#include <stdexcept>
#include <utility>

constexpr auto reverse_bits(std::byte b) -> std::byte
{
  std::byte result{};
  for (auto i = 0; i < CHAR_BIT; ++i) {
    result <<= 1;
    result |= std::byte{(b & std::byte{1}) == std::byte{1}};
    b >>= 1;
  }
  return result;
}

auto main() -> int
{
  using ::boost::ut::expect;
  using ::boost::ut::test;

  namespace huffman = ::starflate::huffman;
  using namespace huffman::literals;

  test("basic") = [] {
    // encoded data from soxofaan/dahuffman readme.rst.
    // We reverse the bits in each byte to match the encoding used in DEFLATE.
    constexpr std::array encoded_bytes = {
        reverse_bits(std::byte{134}),
        reverse_bits(std::byte{124}),
        reverse_bits(std::byte{37}),
        reverse_bits(std::byte{19}),
        reverse_bits(std::byte{105}),
        reverse_bits(std::byte{64})};

    constexpr char eot = {'\4'};
    static constexpr auto code_table =  // clang-format off
      huffman::table{
        huffman::table_contents,
        {std::pair{1_c,     'e'},
                  {01_c,    'i'},
                  {001_c,   'n'},
                  {0001_c,  'q'},
                  {00001_c, 'x'},
                  {00000_c, eot}}
      };  // clang-format on

    constexpr std::array expected = {
        'e', 'x', 'e', 'n', 'e', 'e', 'e', 'e', 'x', 'n',
        'i', 'q', 'n', 'e', 'i', 'e', 'i', 'n', 'i', eot,
    };
    constexpr auto output_buf = [&] {
      std::array<char, expected.size()> output_buf{};
      auto result = decode(code_table, encoded_bytes, output_buf.begin());
      // result should point to the back of output_buf.
      if (output_buf.end() != result) {
        throw std::runtime_error("assertion failed");
      }
      return output_buf;
    }();

    static_assert(output_buf == expected);
  };
}
