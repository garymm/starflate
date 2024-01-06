#include "huffman/huffman.hpp"
#include "huffman/src/utility.hpp"

#include <boost/ut.hpp>

#include <array>
#include <climits>
#include <cstdint>
#include <numeric>
#include <ranges>
#include <vector>

auto main() -> int
{
  using ::boost::ut::aborts;
  using ::boost::ut::eq;
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

  test("default constructible") = [] {
    constexpr auto bits = huffman::bit_span{};

    expect(eq(0, bits.size()));
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

  test("consume") = [](size_t n) {
    static constexpr auto data = huffman::byte_array(0b10101010, 0b01010101);

    static constexpr auto nth_bit = [](auto m) {
      return huffman::bit{std::bitset<CHAR_BIT>{
          std::to_integer<unsigned>(data.at(m / CHAR_BIT))}[m % CHAR_BIT]};
    };

    auto bits = huffman::bit_span{data};
    const auto initial_bits = bits;
    if (std::cmp_less_equal(n, bits.size())) {
      bits.consume(n);
      expect(std::cmp_equal(initial_bits.size() - bits.size(), n));
      expect(std::cmp_equal(CHAR_BIT * data.size() - n, bits.size()));
      if (std::cmp_less(n, initial_bits.size())) {
        expect(nth_bit(n) == bits[0]);
      }
      if (n == 0) {
        expect(initial_bits.byte_data() == bits.byte_data());
      }
    } else {
      expect(aborts([&] { bits.consume(n); }));
    }
  } | std::views::iota(0UZ, 2UZ * (CHAR_BIT + 1UZ));

  test("consume returns reference") = [] {
    static constexpr auto data = std::byte{};

    constexpr auto consumed_size = [] {
      auto bits = huffman::bit_span{&data, CHAR_BIT};
      return bits.consume(1).size();
    }();

    expect(eq(CHAR_BIT - 1, consumed_size));

    constexpr auto consumed_bits =
        huffman::bit_span{&data, CHAR_BIT}.consume(1);
    expect(eq(CHAR_BIT - 1, consumed_bits.size()));
  };

  test("consume_to_byte_boundary") = [] {
    static constexpr auto data = huffman::byte_array(0b10101010, 0b01010101);
    huffman::bit_span span{data};
    const auto initial_span = span;
    expect(span.front() == 0_b);
    expect(initial_span.size() == span.size());
    // should be a no-op now.
    span.consume_to_byte_boundary();
    expect(span.front() == 0_b);
    expect(initial_span.size() == span.size());

    span.consume(1);

    span.consume_to_byte_boundary();
    expect(span.front() == 1_b);
    expect(initial_span.size() - span.size() == CHAR_BIT);
  };

  test("pop") = [] {
    using ::boost::ut::eq;

    // NOLINTBEGIN(readability-magic-numbers)
    static constexpr auto data =
        huffman::byte_array(0b10101010, 0b01010101, 0b11111111);
    huffman::bit_span span{data};
    const std::uint16_t got_16{span.pop_16()};
    constexpr std::uint16_t expected_16{0b0101010110101010};
    expect(eq(got_16, expected_16));

    expect(aborts([&] { span.pop_16(); }));

    const std::uint8_t got_8{span.pop_8()};
    constexpr std::uint8_t expected_8{0b11111111};
    expect(eq(got_8, expected_8));

    expect(aborts([&] { span.pop_8(); }));

    span = huffman::bit_span{data};
    const std::uint16_t got_5{span.pop_n(5)};
    constexpr std::uint16_t expected_5{0b01010};
    expect(eq(got_5, expected_5));

    const std::uint16_t got_3{span.pop_n(3)};
    constexpr std::uint16_t expected_3{0b101};
    expect(eq(got_3, expected_3));
    // NOLINTEND(readability-magic-numbers)
  };
}
