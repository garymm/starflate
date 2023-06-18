#pragma once

#include "huffman/src/bit.hpp"
#include "huffman/src/code.hpp"
#include "huffman/src/detail/adjacent_node_iterator.hpp"
#include "huffman/src/encoding.hpp"

#include <algorithm>
#include <compare>
#include <cstddef>
#include <functional>

namespace gpu_deflate::huffman::detail {

/// A node of a Huffman tree
///
/// This class is used to build a Huffman tree in-place and avoids allocation
/// for internal nodes of a Huffman tree. A `table_node` may represent
/// either a leaf node or an internal node of a Huffman tree.
///
/// On construction, a `table_node` is a leaf node, containing the
/// frequency of the underlying symbol.
///
/// | freq: 1 | freq: 1 | freq: 3 | freq: 4 | freq: 5 |
/// | ns:   1 | ns:   1 | ns:   1 | ns:   1 | ns:   1 |
/// | sym:  A | sym:  B | sym:  C | sym:  D | sym:  E |
///
/// When two nodes are joined (adjacent in the associated container), the left
/// node becomes an internal node (if not already).
///
/// | freq: 2 | freq: 1 | freq: 3 | freq: 4 | freq: 5 |
/// | ns:   2 | ns:   1 | ns:   1 | ns:   1 | ns:   1 |
/// | sym:  A | sym:  B | sym:  C | sym:  D | sym:  E |
///
/// ^^^^^^^^^^ This first element represents an internal node.
///
/// Use of this type is restricted to contiguous containers due to use of
/// pointer arithmetic.
///
/// After completion of the Huffman tree, the encoding for all symbols is
/// obtained by iterating over the associated container's elements and
/// obtained the underlying `encoding` for each element.
///
template <class Symbol>
class table_node : public encoding<Symbol>
{
  std::size_t frequency_{};
  std::size_t node_size_{};

public:
  using encoding_type = encoding<Symbol>;
  using symbol_type = typename encoding_type::symbol_type;

  /// Construct an "empty" intrusive node
  ///
  table_node() = default;

  /// Construct a leaf node for a symbol and its frequency
  ///
  constexpr table_node(symbol_type sym, std::size_t freq)
      : encoding_type{sym}, frequency_{freq}, node_size_{1UZ}
  {}

  constexpr auto frequency() const { return frequency_; }

  constexpr auto node_size() const { return node_size_; }

  /// "Joins" two `table_node`s
  ///
  /// Logically "join" `lhs` with the next adjacent node `rhs`, "creating" an
  /// internal node. This adds the frequency of the `rhs` node to the `lhs`node,
  /// left pads all the codes of the internal nodes of `lhs` with 0s and left
  /// pads all the code of the internal nodes of `rhs` with 1s.
  ///
  /// Logically "join" `*this` with the next adjacent node `*next()`,
  /// "creating" an internal node. This adds the frequency of the next node to
  /// this node, left pads all the codes of the internal nodes of `*this` with
  /// 0s and left pads all the code of the internal nodes of `*next()` with
  /// 1s.
  ///
  /// @pre `lhs` and `rhs` are adjacent
  ///
  static constexpr auto join_adjacent(table_node& lhs, table_node& rhs) -> void
  {
    const auto i = adjacent_node_iterator{&lhs};
    const auto j = adjacent_node_iterator{&rhs};

    assert(
        std::next(i) == j and
        "`lhs` and `rhs` must be adjacent "
        "`table_node` values.");

    const auto k = std::next(j);

    const auto left_pad_with = [](auto b) {
      return [b](table_node& n) { b >> static_cast<code&>(n); };
    };

    std::for_each(i.base(), j.base(), left_pad_with(bit{0}));
    std::for_each(j.base(), k.base(), left_pad_with(bit{1}));

    lhs.frequency_ += rhs.frequency_;
    lhs.node_size_ += rhs.node_size_;
  }

  [[nodiscard]]
  friend constexpr auto
  operator<=>(const table_node& lhs, const table_node& rhs) noexcept
      -> std::strong_ordering
  {
    if (const auto cmp = lhs.frequency() <=> rhs.frequency(); cmp != 0) {
      return cmp;
    }

    return lhs.symbol <=> rhs.symbol;
  }

  [[nodiscard]]
  friend constexpr auto
  operator==(const table_node& lhs, const table_node& rhs) noexcept -> bool
  {
    return (lhs <=> rhs) == 0;
  }
};

}  // namespace gpu_deflate::huffman::detail
