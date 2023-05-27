#pragma once

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
class table_node : encoding<Symbol>
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

  /// Obtain the underlying `encoding` for this node
  ///
  /// @{

  constexpr auto base() -> encoding_type&
  {
    return static_cast<encoding_type&>(*this);
  }
  constexpr auto base() const -> const encoding_type&
  {
    return static_cast<const encoding_type&>(*this);
  }

  /// @}

  /// Obtains the next node, with respect to node size
  ///
  /// Given a contiguous container of nodes, `next()` returns a pointer to
  /// the next node, using `node_size()` to determine if `*this` is an
  /// internal node or a leaf node. If `*this` is an internal node, (i.e.
  /// `*this` represents a node with children) `next()` skips the appropriate
  /// number of elements in the associated container.
  ///
  /// | freq: 3 | freq: 1 | freq: 1 | freq: 4 | freq: 2 |
  /// | ns:   3 | ns:   1 | ns:   1 | ns:   2 | ns:   1 |
  /// ^                             ^
  /// this                          |
  ///                               |
  /// this->next() -----------------+
  ///
  /// @{

  constexpr auto next() -> table_node* { return this + node_size(); }
  constexpr auto next() const -> const table_node*
  {
    return this + node_size();
  }

  /// @}

  /// "Joins" two `table_node`s
  ///
  /// Logically "join" `*this` with the next adjacent node `*next()`,
  /// "creating" an internal node. This adds the frequency of the next node to
  /// this node, left pads all the codes of the internal nodes of `*this` with
  /// 0s and left pads all the code of the internal nodes of `*next()` with
  /// 1s.
  ///
  /// @pre `*this` is not `back()` of the associated container
  ///
  constexpr auto join_with_next() & -> void
  {
    const auto on_base = [](auto f) {
      return [f](table_node& n) { std::invoke(f, n.base()); };
    };
    std::for_each(this, next(), on_base(&encoding_type::pad_with_0));
    std::for_each(next(), next()->next(), on_base(&encoding_type::pad_with_1));

    const auto& n = *next();
    frequency_ += n.frequency();
    node_size_ += n.node_size();
  }

  [[nodiscard]]
  friend constexpr auto
  operator<=>(const table_node& lhs, const table_node& rhs) noexcept
      -> std::strong_ordering
  {
    if (const auto cmp = lhs.frequency() <=> rhs.frequency(); cmp != 0) {
      return cmp;
    }

    return lhs.base().symbol <=> rhs.base().symbol;
  }

  [[nodiscard]]
  friend constexpr auto
  operator==(const table_node& lhs, const table_node& rhs) noexcept -> bool
  {
    return (lhs <=> rhs) == 0;
  }
};

}  // namespace gpu_deflate::huffman::detail
