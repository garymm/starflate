#include "decompress.hpp"

#include <cstdint>
#include <iterator>
#include <utility>

namespace starflate {
namespace detail {

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

}  // namespace detail

auto decompress(std::span<const std::byte> src, std::span<std::byte> dst)
    -> std::expected<DecompressResult, DecompressError>
{
  using enum detail::BlockType;

  huffman::bit_span src_bits{src};
  while (true) {
    const auto header = detail::read_header(src_bits);
    if (not header) {
      return std::unexpected{header.error()};
    }
    if (header->type == NoCompression) {  // no compression
      // Any bits of input up to the next byte boundary are ignored.
      src_bits.consume_to_byte_boundary();
      const std::uint16_t len = src_bits.pop_16();
      const std::uint16_t nlen = src_bits.pop_16();
      if (len != static_cast<std::uint16_t>(~nlen)) {
        return std::unexpected{DecompressError::NoCompressionLenMismatch};
      }
      // TODO: should we return an error instead of assert?
      assert(
          std::cmp_greater_equal(
              src_bits.size(), std::size_t{len} * CHAR_BIT) and
          "not enough bits in src");

      if (std::ranges::size(dst) < len) {
        return DecompressResult{src, dst};
      }

      std::copy_n(src_bits.byte_data(), len, dst.begin());
      src_bits.consume(CHAR_BIT * len);
      dst = dst.subspan(len);
    } else {
      // TODO: implement
      return std::unexpected{DecompressError::Error};
    }
    const auto distance =
        std::distance(std::ranges::data(src), src_bits.byte_data());
    assert(distance >= 0 and "distance must be positive");
    src = src.subspan(static_cast<size_t>(distance));
    if (header->final) {
      break;
    }
  }
  return DecompressResult{src, dst};
}

}  // namespace starflate
