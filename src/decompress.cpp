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
/// @pre n <= 16 for T = uint16_t, n <= 8 for T = uint8_t
///
/// @returns the n bits removed from the beginning of this.
/// The bits are in the lower (rightmost) part of the return value.
///
template <typename T>
auto pop_bits(huffman::bit_span& bits, std::uint8_t n) -> T
{
  if constexpr (std::is_same_v<T, std::uint16_t>) {
    assert(n <= 16);
  } else if constexpr (std::is_same_v<T, std::uint8_t>) {
    assert(n <= 8);
  } else {
    static_assert(
        std::is_same_v<T, std::uint16_t> || std::is_same_v<T, std::uint8_t>,
        "pop_extra_bits only supports uint8_t and uint16_t");
  }
  auto iter = bits.begin();
  T res{};
  for (std::uint8_t i{}; i < n; i++) {
    res |= static_cast<T>(static_cast<T>(static_cast<bool>(*iter)) << i);
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
  const auto extra_len = pop_bits<std::uint16_t>(src_bits, len_info.extra_bits);
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

template <std::size_t Extent>
auto decompress_length_distance(
    std::uint16_t len,
    huffman::bit_span& src_bits,
    std::span<std::byte> dst,
    std::ptrdiff_t& dst_written,
    const huffman::table<std::uint16_t, Extent>& dist_table) -> DecompressStatus
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
      dist_info.base + pop_bits<std::uint16_t>(src_bits, dist_info.extra_bits);
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

template <std::size_t LenExtent, std::size_t DistExtent>
auto decompress_block_huffman(
    huffman::bit_span& src_bits,
    std::span<std::byte> dst,
    std::ptrdiff_t& dst_written,
    const huffman::table<std::uint16_t, LenExtent>& len_table,
    const huffman::table<std::uint16_t, DistExtent>& dist_table)
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

struct DynamicHuffmanTables
{
  huffman::table<std::uint16_t, std::dynamic_extent> len_table;
  huffman::table<std::uint16_t, std::dynamic_extent> dist_table;
};

constexpr std::array<std::uint8_t, 19> code_length_symbols = {
    16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

auto decode_dynamic_huffman_table(
    huffman::bit_span& src_bits,
    const huffman::table<std::uint8_t>& code_length_table,
    std::uint16_t n_codes)
    -> std::expected<huffman::table<std::uint16_t>, DecompressStatus>
{
  constexpr std::uint8_t kRepeatPrevSymbol = 16;
  constexpr std::uint8_t kRepeat0For3BitsSymbol = 17;
  constexpr std::uint8_t kRepeatZeroFor7BitsSymbol = 18;
  std::vector<std::uint8_t> code_bitsizes(n_codes);
  for (std::uint16_t i = 0; i < n_codes; i++) {
    const auto length_code = huffman::decode_one(code_length_table, src_bits);
    if (not length_code.has_value()) {
      return std::unexpected{DecompressStatus::InvalidLitOrLen};
    }
    src_bits.consume(length_code.encoded_size());
    if (length_code.symbol() == 0) {
      code_bitsizes[i] = 0;
    } else if (length_code.symbol() < kRepeatPrevSymbol) {
      code_bitsizes[i] = length_code.symbol();
    } else if (length_code.symbol() == kRepeatPrevSymbol) {
      constexpr std::uint8_t kRepeatCountBits = 2;
      const std::uint8_t repeat_count =
          pop_bits<std::uint8_t>(src_bits, kRepeatCountBits) + 3;
      for (std::uint8_t j = 0; j < repeat_count; j++) {
        code_bitsizes[i + j] = code_bitsizes[i - 1];
      }
      i += repeat_count - 1;
    } else if (length_code.symbol() == kRepeat0For3BitsSymbol) {
      constexpr std::uint8_t kRepeatCountBits = 3;
      const std::uint8_t repeat_count =
          pop_bits<std::uint8_t>(src_bits, kRepeatCountBits) + 3;
      for (std::uint8_t j = 0; j < repeat_count; j++) {
        code_bitsizes[i + j] = 0;
      }
      i += repeat_count - 1;
    } else if (length_code.symbol() == kRepeatZeroFor7BitsSymbol) {
      constexpr std::uint8_t kRepeatCountBits = 7;
      const std::uint8_t repeat_count =
          pop_bits<std::uint8_t>(src_bits, kRepeatCountBits) + 11;
      for (std::uint8_t j = 0; j < repeat_count; j++) {
        code_bitsizes[i + j] = 0;
      }
      i += repeat_count - 1;
    } else {
      return std::unexpected{DecompressStatus::InvalidLitOrLen};
    }
  }
  std::vector<std::pair<huffman::symbol_span<std::uint16_t>, std::uint8_t>>
      symbol_bitsize_pairs{};
  for (std::uint16_t i = 0; i < n_codes; i++) {
    if (code_bitsizes[i] == 0) {
      continue;
    }
    symbol_bitsize_pairs.emplace_back(
        huffman::symbol_span<std::uint16_t>{i}, code_bitsizes[i]);
  }
  return huffman::table<std::uint16_t>{
      huffman::symbol_bitsize, symbol_bitsize_pairs};
}

auto decode_dynamic_huffman_tables(huffman::bit_span& src_bits)
    -> std::expected<DynamicHuffmanTables, DecompressStatus>
{
  // RFC 3.2.7: Dynamic Huffman codes
  constexpr std::uint8_t kHLitBits = 5;
  const auto h_lit = pop_bits<std::uint8_t>(src_bits, kHLitBits);
  const std::uint16_t n_len_codes = 257 + h_lit;

  constexpr std::uint8_t kHDistBits = 5;
  const auto h_dist = pop_bits<std::uint8_t>(src_bits, kHDistBits);
  const std::uint16_t n_dist_codes = 1 + h_dist;

  constexpr std::uint8_t kHCLenBits = 4;
  const auto h_c_len = pop_bits<std::uint8_t>(src_bits, kHCLenBits);
  const std::uint16_t n_c_len_codes = 4 + h_c_len;

  assert(n_c_len_codes <= code_length_symbols.size());
  std::array<std::uint8_t, code_length_symbols.size()> code_length_bitsizes{};
  constexpr std::uint8_t kCodeLengthBits = 3;
  for (std::uint16_t i = 0; i < n_c_len_codes; i++) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    code_length_bitsizes[i] = pop_bits<std::uint8_t>(src_bits, kCodeLengthBits);
  }
  std::vector<std::pair<huffman::symbol_span<std::uint8_t>, std::uint8_t>>
      code_length_symbol_bitsize_pairs{};
  for (std::size_t i = 0; i < code_length_symbols.size(); i++) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    if (code_length_bitsizes[i] == 0) {
      continue;
    }
    code_length_symbol_bitsize_pairs.emplace_back(
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
        huffman::symbol_span<std::uint8_t>{code_length_symbols[i]},
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
        code_length_bitsizes[i]);
  }
  const auto code_length_table = huffman::table<std::uint8_t>{
      huffman::symbol_bitsize, code_length_symbol_bitsize_pairs};

  auto len_table =
      decode_dynamic_huffman_table(src_bits, code_length_table, n_len_codes);
  if (not len_table) {
    return std::unexpected{len_table.error()};
  }

  auto dist_table =
      decode_dynamic_huffman_table(src_bits, code_length_table, n_dist_codes);
  if (not dist_table) {
    return std::unexpected{dist_table.error()};
  }

  return DynamicHuffmanTables{
      .len_table = std::move(*len_table), .dist_table = std::move(*dist_table)};
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
      const auto maybe_tables = detail::decode_dynamic_huffman_tables(src_bits);
      if (not maybe_tables) {
        return maybe_tables.error();
      }
      const auto& tables = *maybe_tables;
      const auto block_status = detail::decompress_block_huffman(
          src_bits, dst, dst_written, tables.len_table, tables.dist_table);
      if (block_status != DecompressStatus::Success) {
        return block_status;
      }
    }
  }
  return DecompressStatus::Success;
}

}  // namespace starflate
