#pragma once

#include "huffman/src/utility.hpp"

#include <ranges>

namespace starflate::huffman {

/// An inclusive span of symbols
///
template <symbol S>
class symbol_span : public std::ranges::view_interface<symbol_span<S>>
{
  S first_;
  S last_;

  using range_type = std::ranges::iota_view<S, S>;

  constexpr auto as_range() const -> range_type
  {
    return range_type{first_, static_cast<S>(last_ + S{1})};
  }

public:
  using symbol_type = S;

  using iterator = std::ranges::iterator_t<range_type>;

  /// Construct a symbol span of a single symbol
  /// @param first symbol
  ///
  constexpr symbol_span(symbol_type first) : symbol_span{first, first} {}

  /// Construct a symbol span from first to last, inclusive
  /// @param first, last inclusive symbol range
  /// @pre first <= last
  ///
  constexpr symbol_span(symbol_type first, symbol_type last)
      : first_{first}, last_{last}
  {
    assert(first <= last);
  }

  [[nodiscard]]
  constexpr auto begin() const -> iterator
  {
    return as_range().begin();
  }

  [[nodiscard]]
  constexpr auto end() const -> iterator
  {
    return as_range().end();
  }
};

}  // namespace starflate::huffman
