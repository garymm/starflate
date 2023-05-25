#pragma once

#include "detail/huffman_storage.hpp"

#include <algorithm>
#include <cassert>
#include <compare>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <iterator>
#include <numeric>
#include <ranges>
#include <span>
#include <tuple>

namespace gpu_deflate {

/// Temporary object used to print the encoding of a `code_point`
///
struct code_type
{
  std::uint8_t bitsize{};
  std::size_t value{};

  friend auto operator<<(std::ostream& os, const code_type& c) -> std::ostream&
  {
    auto bits = c.bitsize;

    while (bits) {
      --bits;
      os << ((1UZ << bits) & c.value ? '0' : '1');
    }

    return os;
  }
};

/// Internal code table element type
///
template <std::regular Symbol>
  requires std::totally_ordered<Symbol>
struct code_point
{
  using symbol_type = Symbol;

  symbol_type symbol{};
  std::uint8_t bitsize{};
  std::size_t value{};

  /// Left pad the code of `*this` with a 0
  ///
  constexpr auto pad_with_0() -> void { ++bitsize; }

  /// Left pad the code of `*this` with a 1
  ///
  constexpr auto pad_with_1() -> void { value += (1UZ << bitsize++); }

  /// Returns the encoding for `*this`
  ///
  [[nodiscard]]
  constexpr auto code() const -> code_type
  {
    return {bitsize, value};
  }

  friend auto
  operator<<(std::ostream& os, const code_point& point) -> std::ostream&
  {
    os << +point.bitsize        //
       << "\t" << point.code()  //
       << "\t" << point.value   //
       << "\t`" << point.symbol << '`';

    return os;
  }

  [[nodiscard]]
  friend auto
  operator<=>(const code_point&, const code_point&) = default;
};

/// Huffman code table
/// @tparam Symbol symbol type
/// @tparam Extent upper bound for alphabet size
///
/// Determines the Huffman code for a collection of symbols.
///
template <std::regular Symbol, std::size_t Extent = std::dynamic_extent>
  requires std::totally_ordered<Symbol>
class code_table
{
  /// A node of a Huffman tree
  ///
  /// This class is used to build a Huffman tree in-place and avoids allocation
  /// for internal nodes of a Huffman tree. An `intrusive_node` may represent
  /// either a leaf node or an internal node of a Huffman tree.
  ///
  /// On construction, an `intrusive_node` is a leaf node, containing the
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
  /// After completion of the Huffman tree, the encoding for all symbols can
  /// be obtained by iterating over the `code_tree`s elements, which are
  /// `code_point`s.
  ///
  class intrusive_node : code_point<Symbol>
  {
    std::size_t frequency_{};
    std::size_t node_size_{};

  public:
    using code_point_type = code_point<Symbol>;
    using symbol_type = typename code_point_type::symbol_type;

    /// Construct an "empty" intrusive node
    ///
    intrusive_node() = default;

    /// Construct a leaf node for a symbol and its frequency
    ///
    constexpr intrusive_node(symbol_type sym, std::size_t freq)
        : code_point_type{sym}, frequency_{freq}, node_size_{1UZ}
    {}

    constexpr auto frequency() const { return frequency_; }

    constexpr auto node_size() const { return node_size_; }

    /// Obtain the underlying `code_point` for this node
    ///
    /// @{

    constexpr auto base() -> code_point_type&
    {
      return static_cast<code_point_type&>(*this);
    }
    constexpr auto base() const -> const code_point_type&
    {
      return static_cast<const code_point_type&>(*this);
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

    constexpr auto next() -> intrusive_node* { return this + node_size(); }
    constexpr auto next() const -> const intrusive_node*
    {
      return this + node_size();
    }

    /// @}

    /// "Joins" two `intrusive_node`s
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
        return [f](intrusive_node& n) { std::invoke(f, n.base()); };
      };
      std::for_each(this, next(), on_base(&code_point_type::pad_with_0));
      std::for_each(
          next(), next()->next(), on_base(&code_point_type::pad_with_1));

      const auto& n = *next();
      frequency_ += n.frequency();
      node_size_ += n.node_size();
    }

    [[nodiscard]]
    friend constexpr auto
    operator<=>(const intrusive_node& lhs, const intrusive_node& rhs) noexcept
        -> std::strong_ordering
    {
      if (const auto cmp = lhs.frequency() <=> rhs.frequency(); cmp != 0) {
        return cmp;
      }

      return lhs.base().symbol <=> rhs.base().symbol;
    }

    [[nodiscard]]
    friend constexpr auto
    operator==(const intrusive_node& lhs, const intrusive_node& rhs) noexcept
        -> bool
    {
      return (lhs <=> rhs) == 0;
    }
  };

  detail::huffman_storage<intrusive_node, Extent> table_;

  // Create a base view member to prevent member call on the temporary created
  // by views::transform
  //
  // @{

  static constexpr auto as_const_base(const intrusive_node& node) -> const
      typename intrusive_node::code_point_type&
  {
    return node.base();
  }
  using base_view_type = decltype(std::views::transform(
      std::declval<decltype((table_))>(), &as_const_base));
  base_view_type base_view_{std::views::transform(table_, &as_const_base)};

  // @}

  constexpr static auto find_node_if(auto first, auto last, auto pred)
  {
    for (; first != last; first = first->next()) {
      if (pred(*first)) {
        break;
      }
    }

    return first;
  }

public:
  /// Code point type
  ///
  using code_point_type = code_point<Symbol>;

  /// Symbol type
  ///
  using symbol_type = typename code_point_type::symbol_type;

  /// Constructs a `code_table` from a symbol-frequency mapping
  /// @tparam R sized-range of symbol-frequency 2-tuples
  /// @param frequencies mapping with symbol frequencies
  /// @param eot end-of-transmission symbol
  /// @pre `eot` is not a symbol in `frequencies`
  /// @pre frequency for a given symbol is positive
  /// @pre `frequencies` does not contain duplicate symbol values
  ///
  /// @{

  template <std::ranges::sized_range R>
    requires std::convertible_to<
        std::ranges::range_value_t<R>,
        std::tuple<symbol_type, std::size_t>>
  constexpr explicit code_table(const R& frequencies)
      : code_table{frequencies, {}}
  {}

  template <std::ranges::sized_range R>
    requires std::convertible_to<
        std::ranges::range_value_t<R>,
        std::tuple<symbol_type, std::size_t>>
  constexpr code_table(const R& frequencies, symbol_type eot)
      : table_{frequencies, eot}
  {
    const auto total_freq = std::accumulate(
        std::cbegin(frequencies),
        std::cend(frequencies),
        1UZ,  // for EOT which we add later.
        [](auto acc, auto kv) { return acc + kv.second; });

    std::ranges::sort(table_);

    assert(
        std::ranges::unique(
            table_,
            [](auto& x, auto& y) { return x.base().symbol == y.base().symbol; })
            .empty() and
        "`frequencies` cannot contain duplicate symbols");

    while (table_.front().node_size() != table_.size()) {
      table_.front().join_with_next();

      const auto last = table_.data() + table_.size();
      const auto has_higher_freq =
          [f = table_.front().frequency()](const auto& n) {
            return n.frequency() > f;
          };

      auto lower = table_.front().next();
      auto upper = find_node_if(lower, last, has_higher_freq);

      // re-sort after creating a new internal node
      std::rotate(&table_.front(), lower, upper);
    }

    assert(total_freq == table_.front().frequency());
    // make sure we don't get a warning about unused variable in release mode
    (void)total_freq;
  }

  /// @}

  /// Return an iterator to the beginning/end
  ///
  /// @{

  [[nodiscard]]
  constexpr auto begin() const
  {
    return base_view_.begin();
  }

  [[nodiscard]]
  constexpr auto end() const
  {
    return base_view_.end();
  }

  /// @}

  friend auto
  operator<<(std::ostream& os, const code_table& table) -> std::ostream&
  {
    os << "Bits\tCode\tValue\tSymbol\n";
    for (const auto& entry : table) {
      os << entry << '\n';
    }
    return os;
  }
};

namespace detail {

template <class T>
concept tuple_like = requires { typename std::tuple_size<T>::type; };

template <class T>
constexpr auto extent_v()
{
  return std::dynamic_extent;
}

template <tuple_like T>
constexpr auto extent_v()
{
  return std::tuple_size_v<T> + 1UZ;  // +1 for EOT
}

}  // namespace detail

template <class R>
  requires (std::tuple_size_v<std::ranges::range_value_t<R>> == 2)
code_table(const R&) -> code_table<
    std::tuple_element_t<0, std::ranges::range_value_t<R>>,
    detail::extent_v<R>()>;

template <class R, class S>
  requires (
      std::tuple_size_v<std::ranges::range_value_t<R>> == 2 and
      std::convertible_to<
          std::tuple_element_t<0, std::ranges::range_value_t<R>>,
          S>)
code_table(const R&, S) -> code_table<S, detail::extent_v<R>()>;

}  // namespace gpu_deflate
