#include "decompress.hpp"

#include <cstdint>

namespace starflate {
namespace detail {
namespace {

auto valid(BlockType type) -> bool
{
  using enum BlockType;
  return type == NoCompression || type == FixedHuffman ||
         type == DynamicHuffman;
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

struct LengthInfo
{
  std::uint8_t extra_bits;
  std::uint16_t base;
};

constexpr auto lit_or_len_end_of_block = std::uint16_t{256};
constexpr auto lit_or_len_max = std::uint16_t{285};
constexpr auto lit_or_len_max_decoded = std::uint16_t{258};

// RFC 3.2.5: Compressed blocks (length and distance codes)
constexpr auto length_infos = std::array<LengthInfo, 28>{
    {{.extra_bits = 0, .base = 3},   {.extra_bits = 0, .base = 4},
     {.extra_bits = 0, .base = 5},   {.extra_bits = 0, .base = 6},
     {.extra_bits = 0, .base = 7},   {.extra_bits = 0, .base = 8},
     {.extra_bits = 0, .base = 9},   {.extra_bits = 0, .base = 10},
     {.extra_bits = 1, .base = 11},  {.extra_bits = 1, .base = 13},
     {.extra_bits = 1, .base = 15},  {.extra_bits = 1, .base = 17},
     {.extra_bits = 2, .base = 19},  {.extra_bits = 2, .base = 23},
     {.extra_bits = 2, .base = 27},  {.extra_bits = 2, .base = 31},
     {.extra_bits = 3, .base = 35},  {.extra_bits = 3, .base = 43},
     {.extra_bits = 3, .base = 51},  {.extra_bits = 3, .base = 59},
     {.extra_bits = 4, .base = 67},  {.extra_bits = 4, .base = 83},
     {.extra_bits = 4, .base = 99},  {.extra_bits = 4, .base = 115},
     {.extra_bits = 5, .base = 131}, {.extra_bits = 5, .base = 163},
     {.extra_bits = 5, .base = 195}, {.extra_bits = 5, .base = 227}}};

constexpr auto distance_infos = std::array<LengthInfo, 30>{
    {{.extra_bits = 0, .base = 1},      {.extra_bits = 0, .base = 2},
     {.extra_bits = 0, .base = 3},      {.extra_bits = 0, .base = 4},
     {.extra_bits = 1, .base = 5},      {.extra_bits = 1, .base = 7},
     {.extra_bits = 2, .base = 9},      {.extra_bits = 2, .base = 13},
     {.extra_bits = 3, .base = 17},     {.extra_bits = 3, .base = 25},
     {.extra_bits = 4, .base = 33},     {.extra_bits = 4, .base = 49},
     {.extra_bits = 5, .base = 65},     {.extra_bits = 5, .base = 97},
     {.extra_bits = 6, .base = 129},    {.extra_bits = 6, .base = 193},
     {.extra_bits = 7, .base = 257},    {.extra_bits = 7, .base = 385},
     {.extra_bits = 8, .base = 513},    {.extra_bits = 8, .base = 769},
     {.extra_bits = 9, .base = 1025},   {.extra_bits = 9, .base = 1537},
     {.extra_bits = 10, .base = 2049},  {.extra_bits = 10, .base = 3073},
     {.extra_bits = 11, .base = 4097},  {.extra_bits = 11, .base = 6145},
     {.extra_bits = 12, .base = 8193},  {.extra_bits = 12, .base = 12289},
     {.extra_bits = 13, .base = 16385}, {.extra_bits = 13, .base = 24577}}};

/// Removes n bits from the beginning of bits and returns them.
///
/// @pre bits contains at least n bits.
/// @pre n <= 16
///
/// @returns the n bits removed from the beginning of this.
/// The bits are in the lower (rightmost) part of the return value.
///
auto pop_extra_bits(huffman::bit_span& bits, std::uint8_t n) -> std::uint16_t
{
  assert(n <= 16);
  auto iter = bits.begin();
  std::uint16_t res{};
  for (std::uint8_t i{}; i < n; i++) {
    res |= static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(static_cast<bool>(*iter)) << i);
    iter += 1;
  }
  bits.consume(n);  // invalidates iter, so must come after the loop
  return res;
}

enum class DecodeLitOrLenStatus : std::uint8_t
{
  EndOfBlock,
  Error,
};

auto decode_lit_or_len(
    std::uint16_t lit_or_len, huffman::bit_span& src_bits) -> std::
    expected<std::variant<std::byte, std::uint16_t>, DecodeLitOrLenStatus>
{
  if (lit_or_len < detail::lit_or_len_end_of_block) {
    return static_cast<std::byte>(lit_or_len);
  }
  if (lit_or_len == detail::lit_or_len_end_of_block) {
    return std::unexpected{DecodeLitOrLenStatus::EndOfBlock};
  }
  if (lit_or_len > detail::lit_or_len_max) {
    return std::unexpected{DecodeLitOrLenStatus::Error};
  }
  if (lit_or_len == detail::lit_or_len_max) {
    return detail::lit_or_len_max_decoded;
  }
  const auto len_code =
      static_cast<size_t>(lit_or_len - detail::lit_or_len_end_of_block - 1);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
  const auto& len_info = detail::length_infos[len_code];
  const auto extra_len = pop_extra_bits(src_bits, len_info.extra_bits);
  return static_cast<std::uint16_t>(len_info.base + extra_len);
}

auto decompress_literal(
    std::byte literal, std::span<std::byte> dst, std::ptrdiff_t& dst_written)
    -> DecompressStatus
{
  if (dst.size() - static_cast<std::size_t>(dst_written) < 1) {
    return DecompressStatus::DstTooSmall;
  }
  dst[static_cast<size_t>(dst_written++)] = literal;
  return DecompressStatus::Success;
}

auto decompress_length_distance(
    std::uint16_t len,
    huffman::bit_span& src_bits,
    std::span<std::byte> dst,
    std::ptrdiff_t& dst_written,
    const huffman::table<std::uint16_t, fixed_dist_table_size>& dist_table)
    -> DecompressStatus
{
  const auto dist_code_huff_decoded = huffman::decode_one(dist_table, src_bits);
  const auto dist_code = dist_code_huff_decoded.symbol();
  if (not dist_code_huff_decoded.has_value()) {
    return DecompressStatus::InvalidDistance;
  }
  src_bits.consume(dist_code_huff_decoded.encoded_size());
  if (dist_code >= detail::distance_infos.size()) {
    return DecompressStatus::InvalidLitOrLen;
  }
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
  const auto& dist_info = detail::distance_infos[dist_code];
  const std::uint16_t distance =
      dist_info.base + pop_extra_bits(src_bits, dist_info.extra_bits);
  if (distance > dst_written) {
    return DecompressStatus::InvalidDistance;
  }
  if (dst.size() - static_cast<std::size_t>(dst_written) < len) {
    return DecompressStatus::DstTooSmall;
  }
  starflate::detail::copy_from_before(distance, dst.begin() + dst_written, len);
  dst_written += len;
  return DecompressStatus::Success;
}

template <class... Ts>
struct overloaded : Ts...
{
  using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

auto decompress_block_huffman(
    huffman::bit_span& src_bits,
    std::span<std::byte> dst,
    std::ptrdiff_t& dst_written,
    const huffman::table<std::uint16_t, fixed_len_table_size>& len_table,
    const huffman::table<std::uint16_t, fixed_dist_table_size>& dist_table)
    -> DecompressStatus
{
  while (true) {
    // There are two levels of encoding:
    // 1. Huffman coding. This is the outer level, which we decode first
    //    using huffman::decode_one.
    // 2. The literal/length code. This is the inner level, which we decode
    //    second using the length_infos and distance_infos arrays.
    const auto lit_or_len_code_huff_decoded =
        huffman::decode_one(len_table, src_bits);
    // If we decide to supoort chunked input, this will no longer be an error.
    if (not lit_or_len_code_huff_decoded.has_value()) {
      return DecompressStatus::InvalidLitOrLen;
    }
    src_bits.consume(lit_or_len_code_huff_decoded.encoded_size());
    const auto maybe_lit_or_len =
        decode_lit_or_len(lit_or_len_code_huff_decoded.symbol(), src_bits);
    if (not maybe_lit_or_len) {
      if (maybe_lit_or_len.error() == DecodeLitOrLenStatus::EndOfBlock) {
        return DecompressStatus::Success;
      }
      return DecompressStatus::InvalidLitOrLen;
    }
    const auto status = std::visit(
        overloaded{
            [&](std::byte literal) -> DecompressStatus {
              return decompress_literal(literal, dst, dst_written);
            },
            [&](std::uint16_t len) -> DecompressStatus {
              return decompress_length_distance(
                  len, src_bits, dst, dst_written, dist_table);
            }},
        maybe_lit_or_len.value());
    if (status != DecompressStatus::Success) {
      return status;
    }
  }
  return DecompressStatus::Success;
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
