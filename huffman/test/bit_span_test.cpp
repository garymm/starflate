#include "huffman/huffman.hpp"

#include <boost/ut.hpp>

#include <array>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <vector>

auto main() -> int
{
  using ::boost::ut::aborts;
  using ::boost::ut::expect;
  using ::boost::ut::test;

  namespace huffman = ::starflate::huffman;
  using namespace huffman::literals;

  test("basic") = [] {
    static constexpr std::array data{std::byte{0b10101010}, std::byte{0xff}};
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

  test("usable with non byte-aligned data") = [] {
    static constexpr auto data =
        std::array{std::byte{0b10101010}, std::byte{0xff}};

    static constexpr auto bit_size = 7;
    static constexpr auto bit_offset = 3;
    constexpr auto bs = huffman::bit_span{data.begin(), bit_size, bit_offset};

    // NOLINTBEGIN(readability-magic-numbers)

    // from first byte
    static_assert(bs[0] == 0_b);
    static_assert(bs[1] == 1_b);
    static_assert(bs[2] == 0_b);
    static_assert(bs[3] == 1_b);
    static_assert(bs[4] == 0_b);

    // from second byte
    expect(huffman::bit_span{data.begin(), bit_size, bit_offset}[5] == 1_b);
    expect(huffman::bit_span{data.begin(), bit_size, bit_offset}[6] == 1_b);

    // NOLINTEND(readability-magic-numbers)
  };

  using ::boost::ut::operator|;

  test("aborts if bit offset too large") = [](auto bit_offset) {
    static constexpr auto data =
        std::array{std::byte{0b10101010}, std::byte{0xff}};

    expect(aborts([&] {
      static constexpr auto bit_size = 7;
      huffman::bit_span{data.begin(), bit_size, bit_offset};
    }));
  } | std::vector<std::uint8_t>{8, 9, 10};  // NOLINT(readability-magic-numbers)

  test("consume") = [] {
    // NOLINTBEGIN(readability-magic-numbers)
    static constexpr std::array data{
        std::byte{0b10101010}, std::byte{0b01010101}};
    huffman::bit_span span{data.data(), data.size() * CHAR_BIT};
    expect(*span.begin() == 1_b);
    expect(span.data() == data.data());
    // should be a no-op now.
    span.consume_to_byte_boundary();
    expect(*span.begin() == 1_b);
    expect(span.data() == data.data());

    span.consume(1);
    expect(*span.begin() == 0_b);
    expect(span.data() == data.data());

    span.consume(1);
    expect(*span.begin() == 1_b);
    expect(span.data() == data.data());

    span.consume_to_byte_boundary();
    expect(*span.begin() == 0_b);
    expect(span.data() != data.data());
    span.consume(2);
    expect(*span.begin() == 0_b);
    span.consume(5);
    expect(*span.begin() == 1_b);

    span.consume(1);
    // span is now empty. Accssing .begin() would be undefined behavior.

    // consume past the end
    expect(aborts([&] { span.consume(1); }));
    // NOLINTEND(readability-magic-numbers)
  };

  test("pop") = [] {
    // NOLINTBEGIN(readability-magic-numbers)
    static constexpr std::array data{
        std::byte{0b10101010}, std::byte{0b01010101}, std::byte{0b11111111}};
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
