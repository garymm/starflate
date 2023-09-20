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

  namespace huffman = ::starflate::huffman;
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

  test("indexable") = [] {
    static constexpr auto data =
        std::array{std::byte{0b10101010}, std::byte{0xff}};
    constexpr auto bs = huffman::bit_span{data};

    // NOLINTBEGIN(readability-magic-numbers)

    static_assert(bs[0] == 1_b);
    static_assert(bs[1] == 0_b);
    static_assert(bs[2] == 1_b);
    static_assert(bs[3] == 0_b);
    static_assert(bs[4] == 1_b);
    static_assert(bs[5] == 0_b);
    static_assert(bs[6] == 1_b);
    static_assert(bs[7] == 0_b);

    expect(bs[8] == 1_b);
    expect(bs[9] == 1_b);
    expect(bs[10] == 1_b);
    expect(bs[11] == 1_b);
    expect(bs[12] == 1_b);
    expect(bs[13] == 1_b);
    expect(bs[14] == 1_b);
    expect(bs[15] == 1_b);

    // NOLINTEND(readability-magic-numbers)
  };
}
