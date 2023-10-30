#include "huffman/src/utility.hpp"
#include "src/decompress.hpp"

#include <boost/ut.hpp>

#include <vector>

template <class... Ts>
constexpr auto byte_vector(Ts... values)
{
  return std::vector<std::byte>{std::byte(values)...};
}

auto main() -> int
{
  using ::boost::ut::eq;
  using ::boost::ut::expect;
  using ::boost::ut::fatal;
  using ::boost::ut::test;
  using namespace starflate;

  test("read_header") = [] -> void {
    huffman::bit_span empty{nullptr, 0, 0};
    expect(detail::read_header(empty).error() ==
           DecompressError::InvalidBlockHeader);

    constexpr auto bad_block_type = huffman::byte_array(0b111);
    huffman::bit_span bad_block_type_span{bad_block_type};
    expect(detail::read_header(bad_block_type_span).error() ==
           DecompressError::InvalidBlockHeader);

    constexpr auto fixed = huffman::byte_array(0b010);
    huffman::bit_span fixed_span{fixed};
    auto header = detail::read_header(fixed_span);
    expect(header.has_value())
        << "got error: " << static_cast<int>(header.error());
    expect(not header->final);
    expect(header->type == detail::BlockType::FixedHuffman)
        << "got type: " << static_cast<int>(header->type);

    constexpr auto no_compression = huffman::byte_array(0b001);
    huffman::bit_span no_compression_span{no_compression};
    header = detail::read_header(no_compression_span);
    expect(header.has_value())
        << "got error: " << static_cast<int>(header.error());
    expect(header->final);
    expect(header->type == detail::BlockType::NoCompression)
        << "got type: " << static_cast<int>(header->type);
  };

  test("no compression") = [] {
    constexpr auto compressed = huffman::byte_array(
        0b001,
        5,
        0,  // len = 5
        ~5,
        ~0,  // nlen = 5
        'h',
        'e',
        'l',
        'l',
        'o');

    const auto expected = byte_vector('h', 'e', 'l', 'l', 'o');

    const auto actual = decompress(compressed);
    expect(fatal(actual.has_value()))
        << "got error code: " << static_cast<std::int32_t>(actual.error());
    expect(fatal(actual->size() == expected.size()));
    expect(*actual == expected);
  };

  test("fixed huffman") = [] {
    constexpr auto compressed = huffman::byte_array(0b101);
    const auto actual = decompress(compressed);
    expect(not actual.has_value());
  };

  test("dynamic huffman") = [] {
    constexpr auto compressed = huffman::byte_array(0b011);
    const auto actual = decompress(compressed);
    expect(not actual.has_value());
  };
};
