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
  constexpr explicit encoding(symbol_type s, code c)
      : ::gpu_deflate::huffman::code{c}, symbol{s}
  {}

  /// Left pad the code of `*this` with a 0
  ///
  constexpr auto pad_with_0() -> void { ++bitsize; }

  /// Left pad the code of `*this` with a 1
  ///
  constexpr auto pad_with_1() -> void { value += (1UZ << bitsize++); }

  /// Returns the encoding for `*this`
  ///
  [[nodiscard]]
  constexpr auto code() const -> ::gpu_deflate::huffman::code
  {
    return static_cast<::gpu_deflate::huffman::code>(*this);
  }

  friend auto
  operator<<(std::ostream& os, const encoding& point) -> std::ostream&
  {
    os << +point.bitsize        //
       << "\t" << point.code()  //
       << "\t" << point.value   //
       << "\t`" << point.symbol << '`';

    return os;
  }

  [[nodiscard]]
  friend auto
  operator<=>(const encoding&, const encoding&) = default;
};

}  // namespace gpu_deflate::huffman
