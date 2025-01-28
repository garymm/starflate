#pragma once
#include "huffman/src/bit_span.hpp"
#include "huffman/src/code.hpp"
#include "huffman/src/table.hpp"
#include "huffman/src/utility.hpp"

#include <iterator>

namespace starflate::huffman {
/// Decodes a bit stream using a code table.
///
/// If a code from \p bits is not found in \p code_table, the
/// decoding returns immediately without reading remaining \p bits.
///
/// @param code_table The code table to use for decoding.
/// @param bits The bit stream to decode.
/// @param output The output iterator to write the decoded symbols to.
///
/// @returns The output iterator after writing the decoded symbols.
/// @tparam Symbol The type of the symbols in the code table.
/// @tparam Extent The extent of the code table.
/// @tparam O The type of the output iterator.
template <symbol Symbol, std::size_t Extent, std::output_iterator<Symbol> O>
constexpr auto
decode(const table<Symbol, Extent>& code_table, bit_span bits, O output) -> O
{
  while (!bits.empty()) {
    auto result = decode_one(code_table, bits);
    if (not result.has_value()) {
      break;
    }
    *output = result.symbol();
    output++;
    bits.consume(result.encoded_size());
  }
  return output;
}

template <symbol Symbol>
class decode_result
{
public:
  static constexpr std::uint8_t kInvalidEncodedSize = 0;

  constexpr decode_result(Symbol symbol, std::uint8_t encoded_size)
      : symbol_{symbol}, encoded_size_{encoded_size}
  {}

  [[nodiscard]]
  constexpr auto has_value() const -> bool
  {
    return encoded_size_ != kInvalidEncodedSize;
  }

  [[nodiscard]]
  constexpr auto symbol() const -> Symbol
  {
    assert(has_value());
    return symbol_;
  }

  [[nodiscard]]
  constexpr auto encoded_size() const -> std::uint8_t
  {
    assert(has_value());
    return encoded_size_;
  }

private:
  Symbol symbol_;
  std::uint8_t encoded_size_;
};

/// Decodes a single symbol from \p bits using \p code_table.
///
/// @param code_table The code table to use for decoding.
/// @param bits The bit stream to decode.
///
/// @returns The decoded symbol and how many bits its code was.
///          If no symbol was found, result.encoded_size == 0.
/// @tparam Symbol The type of the symbols in the code table.
/// @tparam Extent The extent of the code table.
template <symbol Symbol, std::size_t Extent>
constexpr auto
decode_one(const table<Symbol, Extent>& code_table, bit_span bits)
    -> decode_result<Symbol>
{
  code current_code{};
  auto code_table_pos = code_table.begin();
  for (auto bit : bits) {
    current_code << bit;
    auto found = code_table.find(current_code, code_table_pos);
    if (found) {
      return {(*found)->symbol, (*found)->bitsize()};
    }
    if (found.error() == code_table.end()) {
      break;
    }
    code_table_pos = found.error();
  }
  return {Symbol{}, decode_result<Symbol>::kInvalidEncodedSize};
}

}  // namespace starflate::huffman
