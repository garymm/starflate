#include "decompress.hpp"

#include "huffman/huffman.hpp"

#include <expected>

namespace starflate::detail {

auto valid(BlockType type) -> bool
{
  using enum BlockType;
  return type == NoCompression || type == FixedHuffman ||
         type == DynamicHuffman;
}

auto read_header(huffman::bit_span& compressed_bits)
    -> std::expected<BlockHeader, DecompressError>
{
  if (std::ranges::size(compressed_bits) < 3) {
    return std::unexpected{DecompressError::InvalidBlockHeader};
  }
  auto type = static_cast<BlockType>(
      std::uint8_t{static_cast<bool>(compressed_bits[1])} |
      (std::uint8_t{static_cast<bool>(compressed_bits[2])} << 1));
  if (not valid(type)) {
    return std::unexpected{DecompressError::InvalidBlockHeader};
  }
  const bool final{static_cast<bool>(compressed_bits[0])};
  compressed_bits.consume(3);
  return BlockHeader{final, type};
}
}  // namespace starflate::detail
