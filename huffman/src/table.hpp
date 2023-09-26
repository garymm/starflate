#pragma once

#include "huffman/src/detail/element_base_iterator.hpp"
#include "huffman/src/detail/table_node.hpp"
#include "huffman/src/detail/table_storage.hpp"

#include <algorithm>
#include <compare>
#include <concepts>
#include <cstddef>
#include <expected>
#include <functional>
#include <iterator>
#include <numeric>
#include <optional>
#include <ostream>
#include <ranges>
#include <span>
#include <tuple>
#include <utility>

namespace starflate::huffman {

template <class T, std::size_t N>
using c_array = T[N];

/// Huffman code table
/// @tparam Symbol symbol type
/// @tparam Extent upper bound for alphabet size
///
/// Determines the Huffman code for a collection of symbols.
///
/// If `Extent` is `std::dynamic_extent`, the maximum alphabet size is
/// undetermined and `std::vector` is used to store the Huffman tree. Otherwise,
/// `std::array` is used to store the Huffman tree, with the size determined by
/// `Extent`.
///
template <std::regular Symbol, std::size_t Extent = std::dynamic_extent>
  requires std::totally_ordered<Symbol>
class table
{
  using node_type = detail::table_node<Symbol>;

  detail::table_storage<node_type, Extent> table_;

  constexpr static auto find_node_if(auto first, auto last, auto pred)
  {
    for (; first != last; first = first->next()) {
      if (pred(*first)) {
        break;
      }
    }

    return first;
  }

  constexpr auto construct_table() -> void
  {
    std::ranges::sort(table_);

    assert(
        std::ranges::unique(
            table_, [](auto& x, auto& y) { return x.symbol == y.symbol; })
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
  }

public:
  /// Code point type
  ///
  using encoding_type = encoding<Symbol>;

  /// Symbol type
  ///
  using symbol_type = typename encoding_type::symbol_type;

  /// Const iterator type
  ///
  using const_iterator = detail::element_base_iterator<
      // TODO: construct_table builds the table in reverse order.
      // would be nice to fix that so we can get rid of reverse_iterator here.
      std::reverse_iterator<
          typename detail::table_storage<node_type, Extent>::const_iterator>,
      encoding<Symbol>>;

  /// Constructs a `table` from a symbol-frequency mapping
  /// @tparam R sized-range of symbol-frequency 2-tuples
  /// @param frequencies mapping with symbol frequencies
  /// @param eot end-of-transmission symbol
  /// @pre `eot` is not a symbol in `frequencies`
  /// @pre frequency for a given symbol is positive
  ///
  /// @{

  template <std::ranges::sized_range R>
    requires std::convertible_to<
        std::ranges::range_value_t<R>,
        std::tuple<symbol_type, std::size_t>>
  constexpr explicit table(const R& frequencies) : table{frequencies, {}}
  {}

  template <std::ranges::sized_range R>
    requires std::convertible_to<
        std::ranges::range_value_t<R>,
        std::tuple<symbol_type, std::size_t>>
  constexpr table(const R& frequencies, std::optional<symbol_type> eot)
      : table_{detail::frequency_tag{}, frequencies, eot}
  {
    [[maybe_unused]] const auto total_freq = std::accumulate(
        std::cbegin(frequencies),
        std::cend(frequencies),
        std::size_t{eot.has_value()},
        [](auto acc, auto kv) { return acc + kv.second; });

    construct_table();

    assert(total_freq == table_.front().frequency());
  }

  template <std::integral I, auto N>
  constexpr explicit table(
      const c_array<std::pair<symbol_type, I>, N>& frequencies)
      : table{frequencies |
                  std::views::transform(
                      [](auto p) -> std::pair<symbol_type, std::size_t> {
                        assert(p.second > I{});
                        return {p.first, p.second};
                      }),
              {}}
  {}

  /// @}

  /// Constructs a `table` from a sequence of symbols
  /// @tparam R input-range of symbols
  /// @param data input-range of symbols
  /// @param eot end-of-transmission symbol
  /// @pre `eot` is not a symbol in `data`
  ///
  /// @{

  template <std::ranges::input_range R>
    requires std::convertible_to<std::ranges::range_reference_t<R>, symbol_type>
  constexpr explicit table(const R& data) : table{data, {}}
  {}

  template <std::ranges::input_range R>
    requires std::convertible_to<std::ranges::range_reference_t<R>, symbol_type>
  constexpr explicit table(const R& data, std::optional<symbol_type> eot)
      : table_{detail::data_tag{}, data, eot}
  {
    construct_table();
  }

  /// @}

  /// Constructs a `table` from the given code-symbol mapping contents
  /// @tparam R sized-range of code-symbol 2-tuples
  /// @pre all `code` and `symbol` values container in mapping are unique
  /// @pre `code` values are prefix free
  ///
  /// Construct a `table` with explicit contents. This constructor avoids
  /// generation of prefix-free codes for symbols and assumes that the provided
  /// codes have been generated correctly.
  ///
  /// @{

  template <std::ranges::sized_range R>
    requires (
        std::same_as<std::tuple_element_t<0, std::ranges::range_value_t<R>>,
                     code> and
        std::convertible_to<
            std::tuple_element_t<1, std::ranges::range_value_t<R>>,
            symbol_type>)
  constexpr table(table_contents_tag, const R& map)
      : table_{table_contents_tag{}, map}
  {}

  template <std::size_t N>
  constexpr table(
      table_contents_tag, const c_array<std::pair<code, symbol_type>, N>& map)
      : table_{table_contents_tag{}, map}
  {}

  /// @}

  /// Returns an iterator to the first `encoding`
  ///
  /// @note elements are ordered by code bitsize. If multiple elements have the
  ///     same code bitsize, the order is unspecified.
  ///
  [[nodiscard]]
  constexpr auto begin() const -> const_iterator
  {
    return const_iterator{std::make_reverse_iterator(table_.end())};
  }

  /// Returns an iterator past the last `encoding`
  /// @copydetail begin()
  ///
  [[nodiscard]]
  constexpr auto end() const -> const_iterator
  {
    return const_iterator{std::make_reverse_iterator(table_.begin())};
  }

  /// Finds element with specific code within the code table
  /// @param c code to search for
  /// @param pos first element to consider
  ///
  /// Searches from `pos` for an element with code `c`. Elements are sorted
  /// within a code table by code bitsize and search will terminate if an
  /// element is reached with a larger code bitsize.
  ///
  /// @return a `std::expected` containing a value with an iterator to the
  ///     element with code equal to `c` if found. Otherwise, a `std::expected`
  ///     containing an error with an iterator to the first element with code
  ///     bitsize larger than `c` or `end()`.
  ///
  /// @note If the code `c.bitsize()` exceeds the size of any code in `*this`,
  ///     `find` returns `{std::unexpect, end()}`. Callers should check this
  ///     condition to detect invalid inputs or use of an incorrect `table` for
  ///     a given stream of compressed data.
  ///
  [[nodiscard]]
  constexpr auto
  find(code c) const -> std::expected<const_iterator, const_iterator>
  {
    return find(c, begin());
  }
  constexpr auto find(code c, const_iterator pos) const
      -> std::expected<const_iterator, const_iterator>
  {
    using R = std::expected<const_iterator, const_iterator>;

    for (; pos != end() and ((*pos).bitsize() <= c.bitsize()); ++pos) {
      if (static_cast<const code&>(*pos) == c) {
        return R{std::in_place, pos};
      }
    }

    return R{std::unexpect, pos};
  }

  friend auto operator<<(std::ostream& os, const table& table) -> std::ostream&
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
constexpr auto tuple_size_v()
{
  return std::dynamic_extent;
};

template <tuple_like T>
constexpr auto tuple_size_v()
{
  return std::tuple_size_v<T>;
};

template <std::size_t N, class R>
using tuple_arg_t = std::tuple_element_t<N, std::ranges::range_value_t<R>>;

}  // namespace detail

template <class R>
  requires (detail::tuple_size_v<std::ranges::range_value_t<R>>() == 2)
table(const R&) -> table<detail::tuple_arg_t<0, R>, detail::tuple_size_v<R>()>;

template <class R, class S>
  requires (detail::tuple_size_v<std::ranges::range_value_t<R>>() == 2 and
            std::convertible_to<detail::tuple_arg_t<0, R>, S>)
table(const R&, S) -> table<
    S,
    std::max(detail::tuple_size_v<R>(), detail::tuple_size_v<R>() + 1UZ)>;

template <class R, class S>
  requires std::convertible_to<std::ranges::range_reference_t<R>, S>
table(const R&, S) -> table<S>;

template <class R>
  requires (detail::tuple_size_v<std::ranges::range_value_t<R>>() != 2)
table(const R&) -> table<std::ranges::range_value_t<R>>;

template <class S, std::integral I, std::size_t N>
table(const c_array<std::pair<S, I>, N>&) -> table<S, N>;

template <class S, std::size_t N>
table(table_contents_tag, const c_array<std::pair<code, S>, N>&) -> table<S, N>;

template <class R>
  requires (detail::tuple_size_v<std::ranges::range_value_t<R>>() == 2)
table(table_contents_tag, const R&)
    -> table<detail::tuple_arg_t<1, R>, detail::tuple_size_v<R>()>;

}  // namespace starflate::huffman
