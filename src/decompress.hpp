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
  NonCompressedLenMismatch,
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
  for (bool keep_going{true}; keep_going;) {
    const auto header = detail::read_header(compressed_bits);
    if (not header) {
      return std::unexpected{header.error()};
    }
    keep_going = not header->final;
    if (header->type == NoCompression) {  // no compression
      // Any bits of input up to the next byte boundary are ignored.
      compressed_bits.consume_to_byte_boundary();
      const std::uint16_t len = compressed_bits.pop_16();
      const std::uint16_t nlen = compressed_bits.pop_16();
      if (len != static_cast<uint16_t>(~nlen)) {
        return std::unexpected{DecompressError::NonCompressedLenMismatch};
      }

      std::copy_n(
          compressed_bits.data(), len, std::back_inserter(decompressed));
      compressed_bits.consume(CHAR_BIT * len);
    } else {
      // TODO: implement
      return std::unexpected{DecompressError::Error};
    }
  }
  return decompressed;
}

}  // namespace starflate
