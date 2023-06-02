#pragma once

#include "huffman/src/code.hpp"

#include <concepts>
#include <ostream>

namespace gpu_deflate::huffman {

/// A mapping between a symbol and a code
///
/// This type associates a symbol to a code. It is typically constructed and
/// updated as part of the construction of a table.
///
template <std::regular Symbol>
  requires std::totally_ordered<Symbol>
struct encoding : code
{
  using symbol_type = Symbol;

  symbol_type symbol{};

  /// Construct an encoding for a default constructed symbol with an "empty"
  /// code
  ///
  encoding() = default;

  /// Construct an encoding for a symbol with an "empty" code
  ///
  constexpr explicit encoding(symbol_type s) : symbol{s} {}

  /// Construct an encoding for a symbol with a specific code
  ///
  constexpr explicit encoding(symbol_type s, code c) : code{c}, symbol{s} {}

  friend auto
  operator<<(std::ostream& os, const encoding& point) -> std::ostream&
  {
    os << +point.bitsize                           //
       << "\t" << static_cast<const code&>(point)  //
       << "\t" << point.value                      //
       << "\t`" << point.symbol << '`';

    return os;
  }

  [[nodiscard]]
  friend auto
  operator<=>(const encoding&, const encoding&) = default;
};

}  // namespace gpu_deflate::huffman
