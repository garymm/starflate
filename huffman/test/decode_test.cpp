#include "huffman/huffman.hpp"

#include <boost/ut.hpp>

#include <algorithm>
#include <array>
#include <climits>
#include <cstddef>
#include <utility>

namespace {

constexpr auto eot = '\4';

constexpr auto code_table = [] {
  using namespace ::starflate::huffman::literals;

  // clang-format off
  return ::starflate::huffman::table{
      ::starflate::huffman::table_contents,
      {
          std::pair{0_c, 'e'},
                   {10_c, 'i'},
                   {110_c, 'n'},
                   {1110_c, 'q'},
                   {11110_c, eot},
                   {11111_c, 'x'},
      }};
  // clang-format on
}();

}  // namespace

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
auto main() -> int
{
  using ::boost::ut::eq;
  using ::boost::ut::expect;
  using ::boost::ut::test;

  namespace huffman = ::starflate::huffman;

  test("empty") = [] {
    constexpr auto result = [] {
      auto buf = std::array<char, 1>{{char{0}}};

      auto it = huffman::decode(code_table, huffman::bit_span{}, buf.begin());

      return it == buf.begin();
    }();

    expect(result);
  };

  test("just `e` - beginning not byte aligned") = [] {
    static constexpr auto expected = std::array{'e'};

    constexpr auto decoded = [] {
      constexpr auto encoded = std::array<std::byte, 1>{{std::byte{0}}};

      auto buf = std::array<char, expected.size()>{};

      auto it [[maybe_unused]]
      = huffman::decode(
          code_table,
          huffman::bit_span{encoded}.consume(CHAR_BIT - 1),
          buf.begin());

      // NOTE: using assert because expect() doesn't work inside constexpr.
      assert(it == buf.end());
      return buf;
    }();

    expect(eq(expected, decoded));
  };

  test("just `e` - beginning not byte aligned, different padding") = [] {
    static constexpr auto expected = std::array{'e'};

    constexpr auto decoded = [] {
      constexpr auto encoded = std::array{std::byte{0b0111'1111}};

      auto buf = std::array<char, expected.size()>{};

      auto it [[maybe_unused]]
      = huffman::decode(
          code_table,
          huffman::bit_span{encoded}.consume(CHAR_BIT - 1),
          buf.begin());

      assert(it == buf.end());
      return buf;
    }();

    expect(eq(expected, decoded));
  };

  test("`e` x8") = [] {
    static constexpr auto expected = [] {
      auto arr = std::array<char, 8>{};
      std::ranges::fill(arr, 'e');
      return arr;
    }();

    constexpr auto decoded = [] {
      constexpr auto encoded = std::array<std::byte, 1>{};

      auto buf = std::array<char, expected.size()>{};

      auto it [[maybe_unused]]
      = huffman::decode(code_table, encoded, buf.begin());

      assert(it == buf.end());
      return buf;
    }();

    expect(eq(expected, decoded));
  };

  test("invalid code") = [] {
    static constexpr auto different_table = [] {
      using namespace ::starflate::huffman::literals;

      // clang-format off
      return ::starflate::huffman::table{
        ::starflate::huffman::table_contents,
        {
          std::pair{0_c, 'e'},
        }};
      // clang-format on
    }();

    static constexpr auto expected = [] {
      auto arr = std::array<char, 8>{};
      std::ranges::fill(arr, 'e');
      return arr;
    }();

    static constexpr auto encoded =
        std::array{std::byte{0x00U}, std::byte{0xFFU}};

    constexpr auto decoded = [] {
      auto buf = std::array<char, expected.size()>{};

      auto it [[maybe_unused]]
      = huffman::decode(different_table, encoded, buf.begin());

      assert(it == buf.end());
      return buf;
    }();

    expect(eq(expected, decoded));
  };

  test("`nx`") = [] {
    static constexpr auto expected = std::array{'n', 'x'};

    constexpr auto decoded = [] {
      constexpr auto encoded = std::array{std::byte{0b1111'1011}};

      auto buf = std::array<char, expected.size()>{};

      auto it [[maybe_unused]]
      = huffman::decode(code_table, encoded, buf.begin());

      assert(it == buf.end());
      return buf;
    }();

    expect(eq(expected, decoded));
  };

  test("`nxqiee`") = [] {
    static constexpr auto expected = std::array{'n', 'x', 'q', 'i', 'e', 'e'};

    constexpr auto decoded = [] {
      constexpr auto encoded = std::array{
          std::byte{0b1111'1011},  //
          std::byte{0b0001'0111}};

      auto buf = std::array<char, expected.size()>{};

      auto it [[maybe_unused]]
      = huffman::decode(code_table, encoded, buf.begin());

      assert(it == buf.end());
      return buf;
    }();

    expect(eq(expected, decoded));
  };

  test("`nxqi` - end is not byte aligned") = [] {
    static constexpr auto expected = std::array{'n', 'x', 'q', 'i'};

    constexpr auto decoded = [] {
      constexpr auto encoded = std::array{
          std::byte{0b1111'1011},  //
          std::byte{0b0001'0111}   // starts with 2 bits of padding
      };

      auto buf = std::array<char, expected.size()>{};

      auto it [[maybe_unused]]
      = huffman::decode(
          code_table,
          huffman::bit_span{encoded.data(), (encoded.size() * CHAR_BIT) - 2},
          buf.begin());

      assert(it == buf.end());
      return buf;
    }();

    expect(eq(expected, decoded));
  };

  test("`exeneeeexni`") = [] {
    static constexpr auto expected =
        std::array{'e', 'x', 'e', 'n', 'e', 'e', 'e', 'e', 'x', 'n', 'i'};

    constexpr auto decoded = [] {
      constexpr auto encoded = std::array{
          std::byte{0b1011'1110},
          std::byte{0b1100'0001},
          std::byte{0b0101'1111}};

      auto buf = std::array<char, expected.size()>{};

      auto it [[maybe_unused]]
      = huffman::decode(code_table, encoded, buf.begin());

      assert(it == buf.end());
      return buf;
    }();

    expect(eq(expected, decoded));
  };

  test("`exeneeeexniqneiein`") = [] {
    static constexpr auto expected = std::array{
        'e', 'x', 'e', 'n', 'e', 'e', 'e', 'e', 'x', 'n',
        'i', 'q', 'n', 'e', 'i', 'e', 'i', 'n', 'i', eot};

    constexpr auto decoded = [] {
      constexpr auto encoded = std::array{
          std::byte{0b1011'1110},
          std::byte{0b1100'0001},
          std::byte{0b0101'1111},
          std::byte{0b0011'0111},
          std::byte{0b0110'1001},
          std::byte{0b0011'1101}};

      auto buf = std::array<char, expected.size()>{};

      auto it [[maybe_unused]]
      = huffman::decode(
          code_table,
          huffman::bit_span{encoded.data(), (encoded.size() * CHAR_BIT) - 1},
          buf.begin());

      assert(it == buf.end());
      return buf;
    }();

    expect(eq(expected, decoded));
  };
}
