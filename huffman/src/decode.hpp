#pragma once
#include "huffman/src/bit_span.hpp"
#include "huffman/src/code.hpp"
#include "huffman/src/table.hpp"

#include <iterator>
#include <span>

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
template <
    std::regular Symbol,
    std::size_t Extent = std::dynamic_extent,
    std::output_iterator<Symbol> O>
constexpr auto
decode(const table<Symbol, Extent>& code_table, bit_span bits, O output) -> O
{
  code current_code{};
  auto code_table_pos = code_table.begin();
  for (auto bit : bits) {
    current_code << bit;
    auto found = code_table.find(current_code, code_table_pos);
    if (found) {
      *output = (*found)->symbol;
      output++;
      code_table_pos = code_table.begin();
      current_code = code{};
      continue;
    }
    if (found.error() == code_table.end()) {
      break;
    }
    code_table_pos = found.error();
  }
  return output;
}
}  // namespace starflate::huffman
