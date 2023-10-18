#include "huffman/huffman.hpp"
#include "huffman/src/utility.hpp"

#include <boost/ut.hpp>

#include <array>
#include <climits>
#include <cstdint>
#include <numeric>
#include <vector>

auto main() -> int
{
  using ::boost::ut::aborts;
  using ::boost::ut::expect;
  using ::boost::ut::test;

  namespace huffman = ::starflate::huffman;
  using namespace huffman::literals;

  test("basic") = [] {
    static constexpr auto data = huffman::byte_array(0b10101010, 0xff);
    // leave off the last bit of the last byte
    constexpr huffman::bit_span span{data.data(), (data.size() * CHAR_BIT) - 1};
    constexpr std::string_view expected = "010101011111111";
    expect(std::ranges::equal(
        span,
        expected | std::views::transform([](char c) {
          return huffman::bit{c};
        })));
  };

  test("indexable") = [] {
    static constexpr auto data = huffman::byte_array(0b10101010, 0xff);
    constexpr auto bs = huffman::bit_span{data};

    // NOLINTBEGIN(readability-magic-numbers)

    static_assert(bs[0] == 0_b);
    static_assert(bs[1] == 1_b);
    static_assert(bs[2] == 0_b);
    static_assert(bs[3] == 1_b);
    static_assert(bs[4] == 0_b);
    static_assert(bs[5] == 1_b);
    static_assert(bs[6] == 0_b);
    static_assert(bs[7] == 1_b);

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

  test("usable with non byte-aligned data") = [] {
    static constexpr auto data = huffman::byte_array(0b10101010, 0xff);

    static constexpr auto bit_size = 7;
    static constexpr auto bit_offset = 3;
    constexpr auto bs = huffman::bit_span{data.begin(), bit_size, bit_offset};

    // NOLINTBEGIN(readability-magic-numbers)

    // from first byte
    static_assert(bs[0] == 1_b);
    static_assert(bs[1] == 0_b);
    static_assert(bs[2] == 1_b);
    static_assert(bs[3] == 0_b);
    static_assert(bs[4] == 1_b);

    // from second byte
    expect(huffman::bit_span{data.begin(), bit_size, bit_offset}[5] == 1_b);
    expect(huffman::bit_span{data.begin(), bit_size, bit_offset}[6] == 1_b);

    // NOLINTEND(readability-magic-numbers)
  };

  using ::boost::ut::operator|;

  test("aborts if bit offset too large") = [](auto bit_offset) {
    static constexpr auto data = huffman::byte_array(0b10101010, 0xff);

    expect(aborts([&] {
      static constexpr auto bit_size = 7;
      huffman::bit_span{data.begin(), bit_size, bit_offset};
    }));
  } | std::vector<std::uint8_t>{8, 9, 10};  // NOLINT(readability-magic-numbers)

  std::vector<unsigned> n_to_consume(1Z + 2Z * CHAR_BIT);
  std::iota(n_to_consume.begin(), n_to_consume.end(), 0);

  test("consume") = [](auto n) {
    static constexpr auto data = huffman::byte_array(0b10101010, 0b01010101);

    static constexpr auto nth_bit = [](auto m) {
      return huffman::bit{std::bitset<CHAR_BIT>{
          std::to_integer<unsigned>(data[m / CHAR_BIT])}[m % CHAR_BIT]};
    };

    auto bits = huffman::bit_span{data};
    if (n <= data.size() * CHAR_BIT) {
      bits.consume(n);
      expect(bits.n_consumed() == n);
      expect(nth_bit(n) == bits[0]);
      expect(CHAR_BIT * data.size() - n == bits.size());
    } else {
      expect(aborts([&] { bits.consume(n); }));
    }
  } | n_to_consume;

  test("consume_to_byte_boundary") = [] {
    static constexpr auto data = huffman::byte_array(0b10101010, 0b01010101);
    huffman::bit_span span{data.data(), data.size() * CHAR_BIT};
    expect(*span.begin() == 0_b);
    expect(span.n_consumed() == 0);
    // should be a no-op now.
    span.consume_to_byte_boundary();
    expect(*span.begin() == 0_b);
    expect(span.n_consumed() == 0);

    span.consume(1);

    span.consume_to_byte_boundary();
    expect(*span.begin() == 1_b);
    expect(span.n_consumed() == CHAR_BIT);
  };

  test("pop") = [] {
    // NOLINTBEGIN(readability-magic-numbers)
    static constexpr auto data =
        huffman::byte_array(0b10101010, 0b01010101, 0b11111111);
    huffman::bit_span span{data.data(), data.size() * CHAR_BIT};
    std::uint16_t got_16{span.pop_16()};
    std::uint16_t expected_16{0b0101010110101010};
    expect(got_16 == expected_16)
        << "got: " << got_16 << " expected: " << expected_16;

    expect(aborts([&] { span.pop_16(); }));

    std::uint8_t got_8{span.pop_8()};
    std::uint8_t expected_8{0b11111111};
    expect(got_8 == expected_8)
        << "got: " << got_8 << " expected: " << expected_8;

    expect(aborts([&] { span.pop_8(); }));
    // NOLINTEND(readability-magic-numbers)
  };
}
