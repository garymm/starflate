#pragma once

#include "huffman/src/code.hpp"
#include "huffman/src/detail/is_specialization_of.hpp"
#include "huffman/src/symbol_span.hpp"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <numeric>
#include <ranges>
#include <tuple>
#include <utility>

namespace starflate::huffman::detail {

/// Range adaptor that flattens a view of symbol_span-bitsize ranges to a single
///     view
/// @tparam V random access view of symbol_span-bitsize elements
///
/// Flattens a view of symbol_span-bitsize elements. Elements of the flattened
/// view are symbol-code pairs. Note that code values set by this view are
/// *unspecified*. It is expected that view is used in construction of a `table`
/// and that valid codes are set with the `canonicalize()` member.
///
/// Example:
/// ~~~{.cpp}
/// const auto data = std::vector<std::pair<symbol_span<char>, std::uint8_t>>{
///   {{'A', 'C'}, 2},
///   {{'D', 'F'}, 3},
///   {{'G', 'H'}, 4},
/// };
///
/// const auto elements =
/// flattened_symbol_bitsize_view{std::ranges::ref_view{data}};
///
/// for (auto [symbol, code] : elements) {
///   std::cout << symbol << ", " code << '\n';
/// }
///
/// // prints
/// // A, 00
/// // B, 00
/// // C, 00
/// // D, 000
/// // E, 000
/// // F, 000
/// // G, 0000
/// // H, 0000
/// ~~~
///
template <std::ranges::view V>
  requires std::ranges::random_access_range<V> and
           (std::tuple_size_v<std::ranges::range_value_t<V>> == 2) and
           is_specialization_of_v<
               std::tuple_element_t<0, std::ranges::range_value_t<V>>,
               symbol_span> and
           std::convertible_to<
               std::tuple_element_t<1, std::ranges::range_value_t<V>>,
               std::uint8_t>
class flattened_symbol_bitsize_view
    : public std::ranges::view_interface<flattened_symbol_bitsize_view<V>>
{
  V base_;
  std::size_t size_;

public:
  using base_type = V;
  using symbol_span_type =
      std::tuple_element_t<0, std::ranges::range_value_t<base_type>>;
  using symbol_type = typename symbol_span_type::symbol_type;

  /// Iterator for flattened_symbol_bitsize_view
  ///
  class iterator
  {
  public:
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ranges::range_difference_t<V>;
    using reference = std::pair<code, symbol_type>;
    using value_type = reference;
    using pointer = void;

  private:
    const flattened_symbol_bitsize_view* parent_{};
    difference_type outer_index_{};
    std::ranges::range_difference_t<symbol_span_type> inner_index_{};

    template <class R, class T>
    static constexpr auto in_range_cast(T t) -> R
    {
      assert(std::in_range<R>(t));
      return static_cast<R>(t);
    }

  public:
    /// Default constructor
    ///
    iterator()
    {
      // this should never be invoked, but the definition is required
      // for weakly_incrementable until GCC and Clang are fixed
      // https://en.cppreference.com/w/cpp/iterator/weakly_incrementable
      // https://wg21.link/P2325R3
      assert(false);
    }

    /// Construct an iterator to a symbol-bitsize element
    /// @param parent view containing the symbol_span-bitsize sequence
    /// @param outer_index index specifying the symbol_span-bitsize element
    /// @param inner_index index within a symbol_span-bitsize element
    ///
    constexpr iterator(
        const flattened_symbol_bitsize_view& parent,
        // NOLINTBEGIN(bugprone-easily-swappable-parameters)
        std::size_t outer_index,
        std::size_t inner_index)
        // NOLINTEND(bugprone-easily-swappable-parameters)
        : parent_{&parent},
          outer_index_{in_range_cast<difference_type>(outer_index)},
          inner_index_{in_range_cast<
              std::ranges::range_difference_t<symbol_span_type>>(inner_index)}
    {}

    [[nodiscard]]
    constexpr auto operator*() const -> reference
    {
      const auto [symbols, bitsize] = parent_->base()[outer_index_];
      return {code{bitsize, {}}, symbols[inner_index_]};
    }

    constexpr auto operator++() & -> iterator&
    {
      const auto [symbols, _] = parent_->base()[outer_index_];

      // if we've reached the end of a symbol_span
      if (std::cmp_equal(++inner_index_, symbols.size())) {
        // advance to the next symbol_span
        ++outer_index_;
        // and reset the symbol_span index
        inner_index_ = {};
      }

      return *this;
    }

    constexpr auto operator++(int) -> iterator
    {
      auto tmp = *this;
      ++*this;
      return tmp;
    }

    friend constexpr auto
    operator==(const iterator&, const iterator&) -> bool = default;
  };

  constexpr explicit flattened_symbol_bitsize_view(V base)
      : base_{std::move(base)},
        size_{std::accumulate(
            std::ranges::cbegin(base_),
            std::ranges::cend(base_),
            0UZ,
            [](auto acc, const auto& symbol_bitsize) {
              const auto& [symbols, _] = symbol_bitsize;
              return acc + symbols.size();
            })}
  {}

  [[nodiscard]]
  constexpr auto base() const -> base_type
  {
    return base_;
  }

  [[nodiscard]]
  constexpr auto size() const -> std::size_t
  {
    return size_;
  }

  [[nodiscard]]
  constexpr auto begin() const -> iterator
  {
    return {*this, 0UZ, 0UZ};
  }

  [[nodiscard]]
  constexpr auto end() const -> iterator
  {
    return {*this, base().size(), 0UZ};
  }
};

}  // namespace starflate::huffman::detail
