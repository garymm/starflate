#pragma once

#include "huffman/huffman.hpp"

#include <cstddef>
#include <expected>
#include <ranges>
#include <span>

namespace starflate {

// error code enum
enum class DecompressStatus : std::uint8_t
{
  Success,
  Error,  // TODO: remove
  InvalidBlockHeader,
  NoCompressionLenMismatch,
  DstTooSmall,
  SrcTooSmall,
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
    -> std::expected<BlockHeader, DecompressStatus>;
}  // namespace detail

/// Decompresses the given source data into the destination buffer.
///
/// @param src The source data to decompress.
/// @param dst The destination buffer to store the decompressed data.
/// @return A status code indicating the result of the decompression.
///
auto decompress(std::span<const std::byte> src, std::span<std::byte> dst)
    -> DecompressStatus;

template <std::ranges::contiguous_range R>
  requires std::same_as<std::ranges::range_value_t<R>, std::byte>
auto decompress(const R& src, std::span<std::byte> dst)
{
  return decompress(std::span{src.data(), src.size()}, dst);
}

}  // namespace starflate
