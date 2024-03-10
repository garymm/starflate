#pragma once

#include "huffman/huffman.hpp"

#include <array>
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
  InvalidLitOrLen,
  InvalidDistance,
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

struct LengthInfo
{
  std::uint8_t extra_bits;
  std::uint16_t base;
};

extern const huffman::table<std::uint16_t, 288> fixed_table;
extern const std::array<LengthInfo, 28> length_infos;
constexpr auto lit_or_len_end_of_block = std::uint16_t{256};
constexpr auto lit_or_len_max = std::uint16_t{285};
constexpr auto lit_or_len_max_decoded = std::uint16_t{258};

/// Copies n bytes from (dst - distance) to dst, handling overlap by repeating.
///
/// From the standard section 3.2.3:
/// the referenced string may overlap the current position; for example, if the
/// last 2 bytes decoded have values X and Y, a string reference with
/// <length = 5, distance = 2> adds X,Y,X,Y,X to the output stream.
///
/// @pre dst - distance is valid.
void copy_from_before(
    std::uint16_t distance,
    std::span<std::byte>::iterator dst,
    std::uint16_t n);
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
