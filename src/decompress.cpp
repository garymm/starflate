#include "decompress.hpp"

#include "huffman/huffman.hpp"

#include <expected>

namespace starflate::detail {

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

// RFC 3.2.6: static literal/length table
//
// literal/length  bitsize  code
// ==============  =======  =========================
//   0 - 143       8          0011'0000 - 1011'1111
// 144 - 255       9        1'1001'0000 - 1'1111'1111
// 256 - 279       7           000'0000 - 001'0111
// 280 - 287       8          1100'0000 - 1100'0111

constexpr auto fixed_table =  // clang-format off
  huffman::table<std::uint16_t, 288>{
    huffman::symbol_bitsize,
    {{{  0, 143}, 8},
      {{144, 255}, 9},
      {{256, 279}, 7},
      {{280, 287}, 8}}};
// clang-format on

// RFC 3.2.5: Compressed blocks (length and distance codes)
constexpr auto length_infos = std::array<LengthInfo, 28>{
    {{0, 3},  {0, 4},  {0, 5},   {0, 6},   {0, 7},   {0, 8},   {0, 9},
     {0, 10}, {1, 11}, {1, 13},  {1, 15},  {1, 17},  {2, 19},  {2, 23},
     {2, 27}, {2, 31}, {3, 35},  {3, 43},  {3, 51},  {3, 59},  {4, 67},
     {4, 83}, {4, 99}, {4, 115}, {5, 131}, {5, 163}, {5, 195}, {5, 227}}};

constexpr auto distance_infos = std::array<LengtheInfo, 30>{
    {{0, 1},     {0, 2},     {0, 3},      {0, 4},      {1, 5},
     {1, 7},     {2, 9},     {2, 13},     {3, 17},     {3, 25},
     {4, 33},    {4, 49},    {5, 65},     {5, 97},     {6, 129},
     {6, 193},   {7, 257},   {7, 385},    {8, 513},    {8, 769},
     {9, 1025},  {9, 1537},  {10, 2049},  {10, 3073},  {11, 4097},
     {11, 6145}, {12, 8193}, {12, 12289}, {13, 16385}, {13, 24577}}};
}  // namespace starflate::detail
