#pragma once

#include "huffman/huffman.hpp"

#include <array>
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
  InvalidLitOrLen,
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

struct LengthInfo
{
  std::uint8_t extra_bits;
  std::uint8_t base;
};

extern const huffman::table<std::uint16_t, 288> fixed_table;
extern const std::array<LengthInfo, 28> length_infos;
constexpr auto lit_or_len_end_of_block = std::uint16_t{256};
constexpr auto lit_or_len_max = std::uint16_t{285};
constexpr auto lit_or_len_max_decoded = std::uint16_t{258};

}  // namespace detail

using namespace huffman::literals;

/// Decompresses a DEFLATE compressed span of bytes.
///
/// @param compressed The compressed bytes.
/// @param alloc The allocator to use for the decompressed bytes.
///   To control the max output size, use a custom allocator that throws an
///   exception when the max size is exceeded.
///
/// @returns The decompressed bytes, or an error.
/// @tparam N The size of the compressed bytes.
/// @tparam ByteAllocator The allocator to use for the decompressed bytes.
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

      // Using vector::insert instead of std::copy to allow for bulk copying.
      decompressed.insert(
          decompressed.end(),
          compressed_bits.byte_data(),
          // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
          compressed_bits.byte_data() + len);
      compressed_bits.consume(CHAR_BIT * len);
    } else if (header->type == FixedHuffman) {
      for (std::uint16_t lit_or_len{};
           lit_or_len != detail::lit_or_len_end_of_block;) {
        const auto decoded =
            huffman::decode_one(detail::fixed_table, compressed_bits);
        if (not decoded.encoded_size) {
          return std::unexpected{DecompressError::InvalidLitOrLen};
        }
        lit_or_len = decoded.symbol;
        compressed_bits.consume(decoded.encoded_size);
        if (lit_or_len < detail::lit_or_len_end_of_block) {
          decompressed.push_back(static_cast<std::byte>(lit_or_len));
        } else if (lit_or_len == detail::lit_or_len_end_of_block) {
          break;
        } else if (lit_or_len > detail::lit_or_len_max) {
          return std::unexpected{DecompressError::InvalidLitOrLen};
        }
        std::uint16_t len{};
        if (lit_or_len == detail::lit_or_len_max) {
          len = detail::lit_or_len_max_decoded;
        } else {
          const auto len_idx =
              static_cast<size_t>(lit_or_len - detail::lit_or_len_end_of_block);
          // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
          const auto& len_info = detail::length_infos[len_idx];
          len = len_info.base + compressed_bits.pop_n(len_info.extra_bits);
        }
      }
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
