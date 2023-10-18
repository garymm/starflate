#pragma once

#include "huffman/huffman.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>
#include <span>
#include <vector>

namespace starflate {

// error code enum
enum class DecompressError : std::uint8_t
{
  Error,
  InvalidBlockHeader,
  NoCompressionLenMismatch,
};

namespace detail {

enum class BlockType : std::uint8_t
{
  NoCompression,
  FixedHuffman,
  DynamicHuffman,
};

struct BlockHeader
{
  bool final;
  BlockType type;
};

auto read_header(huffman::bit_span& compressed_bits)
    -> std::expected<BlockHeader, DecompressError>;
}  // namespace detail

using namespace huffman::literals;

// Inspired by https://docs.python.org/3/library/zlib.html#zlib.decompress
template <std::size_t N, class ByteAllocator = std::allocator<std::byte>>
auto decompress(
    std::span<const std::byte, N> compressed, ByteAllocator alloc = {})
    -> std::expected<std::vector<std::byte, ByteAllocator>, DecompressError>
{

  using enum detail::BlockType;
  auto decompressed = std::vector<std::byte, ByteAllocator>(alloc);

  huffman::bit_span compressed_bits{compressed};
  while (true) {
    const auto header = detail::read_header(compressed_bits);
    if (not header) {
      return std::unexpected{header.error()};
    }
    if (header->type == NoCompression) {  // no compression
      // Any bits of input up to the next byte boundary are ignored.
      compressed_bits.consume_to_byte_boundary();
      const std::uint16_t len = compressed_bits.pop_16();
      const std::uint16_t nlen = compressed_bits.pop_16();
      if (len != static_cast<std::uint16_t>(~nlen)) {
        return std::unexpected{DecompressError::NoCompressionLenMismatch};
      }
      assert(
          std::cmp_greater_equal(
              compressed_bits.size(), std::size_t{len} * CHAR_BIT) and
          "not enough bits");

      // TODO: this is probably really slow because back_inserter means we can
      // only copy a single byte at a time. We should look into options for bulk
      // copying.
      std::copy_n(
          compressed_bits.byte_data(), len, std::back_inserter(decompressed));
      compressed_bits.consume(CHAR_BIT * len);
    } else {
      // TODO: implement
      return std::unexpected{DecompressError::Error};
    }
    if (header->final) {
      break;
    }
  }
  return decompressed;
}

template <
    std::ranges::contiguous_range R,
    class ByteAllocator = std::allocator<std::byte>>
  requires std::same_as<std::ranges::range_value_t<R>, std::byte>
auto decompress(const R& compressed, ByteAllocator alloc = {})
{
  return decompress(std::span{compressed.data(), compressed.size()}, alloc);
}

}  // namespace starflate
