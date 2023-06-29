#include "huffman/src/bit_span.hpp"
#include "huffman/src/code.hpp"
#include "huffman/src/table.hpp"

namespace gpu_deflate::huffman {
template <
    std::regular Symbol,
    std::size_t Extent = std::dynamic_extent,
    class O>
auto decode(const table<Symbol, Extent>& code_table, bit_span bits, O output)
    -> O
{
  code current_code{};
  auto code_table_pos = code_table.begin();
  for (auto bit : bits) {
    current_code << bit;
    auto found = code_table.find(current_code, code_table_pos);
    if (found) {
      *output = found->symbol;
      output++;
      code_table_pos = code_table.begin();
      current_code = code{};
      continue;
    }
    if (found.error() == code_table.end()) {
      assert(false);  // TODO: figure out what a nice API is.
    }
    code_table_pos = found.error();
  }
}
}  // namespace gpu_deflate::huffman
