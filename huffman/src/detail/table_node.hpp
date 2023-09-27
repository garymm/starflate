#pragma once

#include "huffman/src/bit.hpp"
#include "huffman/src/code.hpp"
#include "huffman/src/encoding.hpp"

#include <algorithm>
#include <compare>
#include <cstddef>
#include <functional>

namespace starflate::huffman::detail {

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
/// obtaining the underlying `encoding` for each element.
///
/// Additionally on completion of the Huffman tree, encodings (as well the
/// parent node types) are ordered by symbol bitsize; and table node replaces
/// use of `frequency` and `node_size` with a `skip` field. This skip field
/// provides the distance to the next group of symbols with a larger bitsize.
///
/// | skip: 1 | skip: 2 | skip: 1 | skip: 2 | skip: 1 |
/// |         |         |         |         |         |
/// | sym:  A | sym:  B | sym:  C | sym:  D | sym:  E |
/// | bs:   1 | bs :  2 | bs :  2 | bs :  3 | bs :  3 |
///
template <class Symbol>
class table_node : public encoding<Symbol>
{
  // Data used during initialization of a table to represent a tree
  struct Init
  {
    std::size_t frequency{};
    std::size_t node_size{};
  };

  // Data used during lookup
  struct Decode
  {
    std::size_t skip{};
  };

  union
  {
    Init init_;
    Decode decode_;
  };

public:
  using encoding_type = encoding<Symbol>;
  using symbol_type = typename encoding_type::symbol_type;

  /// Construct an "empty" intrusive node
  ///
  constexpr table_node() : init_{} {}

  /// Construct a leaf node for a symbol and its frequency
  ///
  constexpr table_node(symbol_type sym, std::size_t freq)
      : encoding_type{sym}, init_{.frequency = freq, .node_size = 1UZ}
  {}

  /// Initialization phase member functions
  ///
  /// @{

  constexpr auto frequency() const
  {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    return init_.frequency;
  }

  /// Distance to next internal node
  ///
  /// Given a contiguous container of nodes, `node_size()` returns the distance
  /// to the next internal node.
  ///
  /// | freq: 3 | freq: 1 | freq: 1 | freq: 4 | freq: 2 |
  /// | ns:   3 | ns:   1 | ns:   1 | ns:   2 | ns:   1 |
  /// ^                             ^
  /// this                          |
  ///                               |
  /// next internal node -----------+
  ///
  constexpr auto node_size() const
  {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    return init_.node_size;
  }
  /// "Joins" two `table_node`s
  /// @param lhs left table_node
  /// @param rhs right table_node
  /// @pre `&lhs + lhs.node_size() == &rhs`
  ///
  /// Logically "join" `lhs` with the next adjacent node `rhs` "creating" an
  /// internal node. This adds the frequency of `rhs` to `lhs`, left pads all
  /// the codes of the internal nodes of `lhs` with 0s and left pads all the
  /// code of the internal nodes of `rhs` with 1s.
  ///
  friend constexpr auto join(table_node& lhs, table_node& rhs) -> void
  {
    assert(
        &lhs + lhs.node_size() == &rhs and "`lhs` and `rhs` are not adjacent");

    const auto left_pad_with = [](auto b) {
      return [b](table_node& n) { b >> static_cast<code&>(n); };
    };

    std::for_each(&lhs, &rhs, left_pad_with(bit{0}));
    std::for_each(&rhs, &rhs + rhs.node_size(), left_pad_with(bit{1}));

    // NOLINTBEGIN(cppcoreguidelines-pro-type-union-access)
    lhs.init_.frequency += rhs.frequency();
    lhs.init_.node_size += rhs.node_size();
    // NOLINTEND(cppcoreguidelines-pro-type-union-access)
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

  /// @}

  /// Decode phase member functions
  ///
  /// @{

  constexpr auto set_skip(std::size_t n) -> void
  {
    // https://github.com/llvm/llvm-project/issues/57669
#if __clang__
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    init_.node_size = n;
#else
    decode_ = {.skip = n};
#endif
  }

  /// Returns the number of elements to advance to a node with a larger bitsize.
  [[nodiscard]]
  constexpr auto skip() const -> std::size_t
  {
    // https://github.com/llvm/llvm-project/issues/57669
#if __clang__
    return node_size();
#else
    return decode_.skip;
#endif
  }

  /// @}
};

}  // namespace starflate::huffman::detail
