#include "src/decompress.hpp"

#include <boost/ut.hpp>

#include <array>

auto main() -> int
{
  using ::boost::ut::expect;
  using ::boost::ut::fatal;
  using ::boost::ut::test;
  using namespace starflate;
  test("read_header") = [] -> void {
    huffman::bit_span empty{nullptr, 0, 0};
    expect(detail::read_header(empty).error() ==
           DecompressError::InvalidBlockHeader);

    constexpr auto bad_block_type = std::array{std::byte{0b11100000}};
    huffman::bit_span bad_block_type_span{
        bad_block_type.data(), bad_block_type.size() * CHAR_BIT, 0};
    expect(detail::read_header(bad_block_type_span).error() ==
           DecompressError::InvalidBlockHeader);

    constexpr auto good = std::array{std::byte{0b01000000}};
    huffman::bit_span good_span{good.data(), good.size() * CHAR_BIT, 0};
    const auto header = detail::read_header(good_span);
    expect(header.has_value())
        << "got error: " << static_cast<int>(header.error());
    expect(not header.value().final);
    expect(header.value().type == detail::BlockType::FixedHuffman)
        << "got type: " << static_cast<int>(header.value().type);
  };

  test("no compression block type") = [] {
    constexpr auto compressed = std::array{
        std::byte{0b10011111},
        std::byte{5},
        std::byte{0},  // len = 5
        ~std::byte{5},
        ~std::byte{0},  // nlen = 5
        std::byte{'h'},
        std::byte{'e'},
        std::byte{'l'},
        std::byte{'l'},
        std::byte{'o'}};

    const auto expected = std::vector{
        std::byte{'h'},
        std::byte{'e'},
        std::byte{'l'},
        std::byte{'l'},
        std::byte{'o'}};

    const auto actual =
        decompress(std::span{compressed.data(), compressed.size()});
    expect(fatal(actual.has_value()))
        << "got error code: " << static_cast<std::int32_t>(actual.error());
    const auto& actual_value = actual.value();
    expect(fatal(actual_value.size() == expected.size()));
    expect(actual_value == expected);
  };
};
