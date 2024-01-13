#pragma once

#include "huffman/huffman.hpp"

#include <cstddef>
#include <expected>
#include <ranges>
#include <span>

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

/// @struct DecompressResult
/// @brief A structure representing the result of a decompression operation.
///
struct DecompressResult
{
  std::span<const std::byte> remaining_src;  ///< Remaining source data after
                                             ///< decompression.
  std::span<std::byte> remaining_dst;  ///< Remaining space in the destination
                                       ///< buffer after decompression.
};

/// Decompresses the given source data into the destination buffer.
///
/// @param src The source data to decompress.
/// @param dst The destination buffer to store the decompressed data.
/// @return An expected value containing the decompression result if successful,
/// or an error code if failed.
///
auto decompress(std::span<const std::byte> src, std::span<std::byte> dst)
    -> std::expected<DecompressResult, DecompressError>;

template <std::ranges::contiguous_range R>
  requires std::same_as<std::ranges::range_value_t<R>, std::byte>
auto decompress(const R& src, std::span<std::byte> dst)
{
  return decompress(std::span{src.data(), src.size()}, dst);
}

}  // namespace starflate
