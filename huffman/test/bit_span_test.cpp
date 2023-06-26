#include "huffman/huffman.hpp"

#include <boost/ut.hpp>

#include <array>
#include <climits>
#include <cstddef>
#include <cstdint>

auto main() -> int
{
  using ::boost::ut::aborts;
  using ::boost::ut::expect;
  using ::boost::ut::test;

  namespace huffman = ::gpu_deflate::huffman;
  using namespace huffman::literals;

  test("basic") = [] {
    static constexpr std::array<std::byte, 2> data{
        std::byte{0b10101010}, std::byte{0xff}};
    // leave off the last bit of the last byte
    constexpr huffman::bit_span span{data.data(), (data.size() * CHAR_BIT) - 1};
    constexpr std::string_view expected = "101010101111111";
    expect(std::ranges::equal(
        span,
        expected | std::views::transform([](char c) {
          return huffman::bit{c};
        })));
  };
}
