#include "decompress.hpp"

#include <cstdint>
#include <iostream>
#include <iterator>
#include <utility>

namespace starflate {
namespace detail {
namespace {

auto valid(BlockType type) -> bool
{
  using enum BlockType;
  return type == NoCompression || type == FixedHuffman ||
         type == DynamicHuffman;
}
}  // namespace

auto read_header(huffman::bit_span& compressed_bits)
    -> std::expected<BlockHeader, DecompressStatus>
{
  if (std::ranges::size(compressed_bits) < 3) {
    return std::unexpected{DecompressStatus::InvalidBlockHeader};
  }
  auto type = static_cast<BlockType>(
      std::uint8_t{static_cast<bool>(compressed_bits[1])} |
      (std::uint8_t{static_cast<bool>(compressed_bits[2])} << 1));
  if (not valid(type)) {
    return std::unexpected{DecompressStatus::InvalidBlockHeader};
  }
  const bool final{static_cast<bool>(compressed_bits[0])};
  compressed_bits.consume(3);
  return BlockHeader{.final = final, .type = type};
}

// RFC 3.2.6: static literal/length table
//
// literal/length  bitsize  code
// ==============  =======  =========================
//   0 - 143       8          0011'0000 - 1011'1111
// 144 - 255       9        1'1001'0000 - 1'1111'1111
// 256 - 279       7           000'0000 - 001'0111
// 280 - 287       8          1100'0000 - 1100'0111

constexpr std::size_t fixed_len_table_size = 288;

constexpr auto fixed_len_table =  // clang-format off
  huffman::table<std::uint16_t, fixed_len_table_size>{
    huffman::symbol_bitsize,
    {{{  0, 143}, 8},
      {{144, 255}, 9},
      {{256, 279}, 7},
      {{280, 287}, 8}}};
// clang-format on

constexpr std::size_t fixed_dist_table_size = 32;

constexpr auto fixed_dist_table = huffman::table<
    std::uint16_t,
    fixed_dist_table_size>{huffman::symbol_bitsize, {{{0, 31}, 5}}};

// RFC 3.2.5: Compressed blocks (length and distance codes)
constexpr auto length_infos = std::array<LengthInfo, 28>{
    {{0, 3},  {0, 4},  {0, 5},   {0, 6},   {0, 7},   {0, 8},   {0, 9},
     {0, 10}, {1, 11}, {1, 13},  {1, 15},  {1, 17},  {2, 19},  {2, 23},
     {2, 27}, {2, 31}, {3, 35},  {3, 43},  {3, 51},  {3, 59},  {4, 67},
     {4, 83}, {4, 99}, {4, 115}, {5, 131}, {5, 163}, {5, 195}, {5, 227}}};

constexpr auto distance_infos = std::array<LengthInfo, 30>{
    {{0, 1},     {0, 2},     {0, 3},      {0, 4},      {1, 5},
     {1, 7},     {2, 9},     {2, 13},     {3, 17},     {3, 25},
     {4, 33},    {4, 49},    {5, 65},     {5, 97},     {6, 129},
     {6, 193},   {7, 257},   {7, 385},    {8, 513},    {8, 769},
     {9, 1025},  {9, 1537},  {10, 2049},  {10, 3073},  {11, 4097},
     {11, 6145}, {12, 8193}, {12, 12289}, {13, 16385}, {13, 24577}}};

enum class ParseLitOrLenStatus : std::uint8_t
{
  EndOfBlock,
  Error,
};

auto parse_lit_or_len(
    std::uint16_t lit_or_len, huffman::bit_span& src_bits) -> std::
    expected<std::variant<std::byte, std::uint16_t>, ParseLitOrLenStatus>
{
  if (lit_or_len < detail::lit_or_len_end_of_block) {
    return static_cast<std::byte>(lit_or_len);
  }
  if (lit_or_len == detail::lit_or_len_end_of_block) {
    return std::unexpected{ParseLitOrLenStatus::EndOfBlock};
  }
  if (lit_or_len > detail::lit_or_len_max) {
    return std::unexpected{ParseLitOrLenStatus::Error};
  }
  std::uint16_t len{};
  if (lit_or_len == detail::lit_or_len_max) {
    len = detail::lit_or_len_max_decoded;
  } else {
    const auto len_idx =
        static_cast<size_t>(lit_or_len - detail::lit_or_len_end_of_block - 1);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    const auto& len_info = detail::length_infos[len_idx];
    const auto extra_len = src_bits.pop_n(len_info.extra_bits);
    len = len_info.base + extra_len;
  }
  return len;
}

auto decompress_block_huffman(
    huffman::bit_span& src_bits,
    std::span<std::byte> dst,
    std::ptrdiff_t& dst_written,
    const huffman::table<std::uint16_t, fixed_len_table_size>& len_table,
    const huffman::table<std::uint16_t, fixed_dist_table_size>& dist_table)
    -> DecompressStatus
{
  while (true) {
    const auto lit_or_len_decoded = huffman::decode_one(len_table, src_bits);
    if (not lit_or_len_decoded.encoded_size) {
      return DecompressStatus::InvalidLitOrLen;
    }
    src_bits.consume(lit_or_len_decoded.encoded_size);
    const auto maybe_lit_or_len =
        parse_lit_or_len(lit_or_len_decoded.symbol, src_bits);
    if (not maybe_lit_or_len) {
      if (maybe_lit_or_len.error() == ParseLitOrLenStatus::EndOfBlock) {
        return DecompressStatus::Success;
      }
      return DecompressStatus::InvalidLitOrLen;
    }
    const auto lit_or_len = maybe_lit_or_len.value();
    if (std::holds_alternative<std::byte>(lit_or_len)) {
      if (dst.size() - static_cast<std::size_t>(dst_written) < 1) {
        return DecompressStatus::DstTooSmall;
      }
      dst[static_cast<size_t>(dst_written++)] = std::get<std::byte>(lit_or_len);
      continue;
    }
    const auto len = std::get<std::uint16_t>(lit_or_len);
    const auto dist_decoded = huffman::decode_one(dist_table, src_bits);
    const auto dist_code = dist_decoded.symbol;
    if (not dist_decoded.encoded_size) {
      return DecompressStatus::InvalidDistance;
    }
    src_bits.consume(dist_decoded.encoded_size);
    if (dist_code >= detail::distance_infos.size()) {
      return DecompressStatus::InvalidLitOrLen;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    const auto& dist_info = detail::distance_infos[dist_code];
    const std::uint16_t distance =
        dist_info.base + src_bits.pop_n(dist_info.extra_bits);
    if (distance > dst_written) {
      return DecompressStatus::InvalidDistance;
    }
    if (dst.size() - static_cast<std::size_t>(dst_written) < len) {
      return DecompressStatus::DstTooSmall;
    }
    starflate::detail::copy_from_before(
        distance, dst.begin() + dst_written, len);
    dst_written += len;
  }
  return DecompressStatus::Success;
}

/// Copy n bytes from distance bytes before dst to dst.
void copy_from_before(
    std::uint16_t distance, std::span<std::byte>::iterator dst, std::uint16_t n)
{
  std::ptrdiff_t n_signed{n};
  const auto src = dst - distance;
  while (n_signed > 0) {
    const auto n_to_copy = std::min(n_signed, dst - src);
    dst = std::copy_n(src, n_to_copy, dst);
    n_signed -= n_to_copy;
  }
}

}  // namespace detail

auto decompress(std::span<const std::byte> src, std::span<std::byte> dst)
    -> DecompressStatus
{
  using enum detail::BlockType;

  huffman::bit_span src_bits{src};
  // will always be > 0, but signed type to minimize conversions.
  std::ptrdiff_t dst_written{};
  for (bool was_final = false; not was_final;) {
    const auto header = detail::read_header(src_bits);
    if (not header) {
      return header.error();
    }
    was_final = header->final;
    if (header->type == NoCompression) {  // no compression
      // Any bits of input up to the next byte boundary are ignored.
      src_bits.consume_to_byte_boundary();
      const std::uint16_t len = src_bits.pop_16();
      const std::uint16_t nlen = src_bits.pop_16();
      if (len != static_cast<std::uint16_t>(~nlen)) {
        return DecompressStatus::NoCompressionLenMismatch;
      }
      // Surprisingly size() does not return size_t on libstdc++ 13, hence cast.
      if (static_cast<size_t>(src_bits.size()) <
          std::size_t{len} * std::size_t{CHAR_BIT}) {
        return DecompressStatus::SrcTooSmall;
      }

      if (dst.size() - static_cast<std::size_t>(dst_written) < len) {
        return DecompressStatus::DstTooSmall;
      }

      std::copy_n(src_bits.byte_data(), len, dst.begin() + dst_written);
      src_bits.consume(CHAR_BIT * len);
      dst_written += len;
    } else if (header->type == FixedHuffman) {
      const auto block_status = detail::decompress_block_huffman(
          src_bits,
          dst,
          dst_written,
          detail::fixed_len_table,
          detail::fixed_dist_table);
      if (block_status != DecompressStatus::Success) {
        return block_status;
      }
    } else {
      // TODO: implement
      return DecompressStatus::Error;
    }
  }
  return DecompressStatus::Success;
}

}  // namespace starflate
