#pragma once

#include "huffman/src/detail/table_node.hpp"
#include "huffman/src/detail/table_storage.hpp"

#include <algorithm>
#include <compare>
#include <concepts>
#include <cstddef>
#include <functional>
#include <numeric>
#include <optional>
#include <ostream>
#include <ranges>
#include <span>
#include <tuple>
#include <utility>

namespace gpu_deflate::huffman {

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

  // Create a base view member to prevent member call on the temporary created
  // by views::transform
  //
  // @{

  static constexpr auto as_const_base(const node_type& node) -> const
      typename node_type::encoding_type&
  {
    return node.base();
  }
  using base_view_type = decltype(std::views::reverse(std::views::transform(
      std::declval<decltype((table_))>(), &as_const_base)));
  base_view_type base_view_{
      std::views::reverse(std::views::transform(table_, &as_const_base))};

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

  constexpr auto construct_table() -> void
  {
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
  }

public:
  /// Code point type
  ///
  using encoding_type = encoding<Symbol>;

  /// Symbol type
  ///
  using symbol_type = typename encoding_type::symbol_type;

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
    const auto total_freq = std::accumulate(
        std::cbegin(frequencies),
        std::cend(frequencies),
        std::size_t{eot.has_value()},
        [](auto acc, auto kv) { return acc + kv.second; });

    construct_table();

    assert(total_freq == table_.front().frequency());
    (void)total_freq;
  }

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

template <class R>
using symbol_type_t = std::tuple_element_t<0, std::ranges::range_value_t<R>>;

}  // namespace detail

template <class R>
  requires (detail::tuple_size_v<std::ranges::range_value_t<R>>() == 2)
table(const R&) -> table<detail::symbol_type_t<R>, detail::tuple_size_v<R>()>;

template <class R, class S>
  requires (detail::tuple_size_v<std::ranges::range_value_t<R>>() == 2 and
            std::convertible_to<detail::symbol_type_t<R>, S>)
table(const R&, S) -> table<
    S,
    std::max(detail::tuple_size_v<R>(), detail::tuple_size_v<R>() + 1UZ)>;

template <class R, class S>
  requires std::convertible_to<std::ranges::range_reference_t<R>, S>
table(const R&, S) -> table<S>;

template <class R>
  requires (detail::tuple_size_v<std::ranges::range_value_t<R>>() != 2)
table(const R&) -> table<std::ranges::range_value_t<R>>;

}  // namespace gpu_deflate::huffman
