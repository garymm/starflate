#pragma once

#include "huffman/src/detail/element_base_iterator.hpp"
#include "huffman/src/detail/flattened_symbol_bitsize_view.hpp"
#include "huffman/src/detail/is_specialization_of.hpp"
#include "huffman/src/detail/table_node.hpp"
#include "huffman/src/detail/table_storage.hpp"
#include "huffman/src/symbol_span.hpp"
#include "huffman/src/utility.hpp"

#include <algorithm>
#include <compare>
#include <concepts>
#include <cstddef>
#include <expected>
#include <functional>
#include <iterator>
#include <limits>
#include <numeric>
#include <optional>
#include <ostream>
#include <ranges>
#include <span>
#include <tuple>
#include <utility>

namespace starflate::huffman {

namespace detail {

/// Convert an unsigned integer to signed
///
template <std::signed_integral S, std::unsigned_integral U>
static constexpr auto to_signed(U uint)
{
  assert(std::cmp_less_equal(uint, std::numeric_limits<S>::max()));
  return static_cast<S>(uint);
}

/// Finds the next internal node that satifies a predicate
///
template <std::random_access_iterator I, std::indirect_unary_predicate<I> P>
constexpr static auto find_node_if(I first, I last, P pred)
{
  using S = std::iter_difference_t<I>;

  for (; first != last; first += to_signed<S>(first->node_size())) {
    if (pred(*first)) {
      break;
    }
  }

  return first;
}

}  // namespace detail

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
/// This type uses "DEFLATE" canonical form for codes:
/// * All codes of a given bit length have lexicographically consecutive
///   values, in the same order as the symbols they represent;
/// * Shorter codes lexicographically precede longer codes.
///
template <symbol Symbol, std::size_t Extent = std::dynamic_extent>
class table
{
  using node_type = detail::table_node<Symbol>;

  detail::table_storage<node_type, Extent> table_;

  constexpr auto encode_symbols() -> void
  {
    using S = std::ranges::range_difference_t<decltype(table_)>;
    auto reversed = std::views::reverse(table_);

    // precondition, audit
    assert(std::ranges::is_sorted(reversed));

    const auto total_size = reversed.size();
    auto first = reversed.begin();
    const auto last = reversed.end();

    while (first->node_size() != total_size) {
      join_reversed(first[0], first[detail::to_signed<S>(first->node_size())]);

      const auto has_higher_freq = [&first](const auto& n) {
        return n.frequency() > first->frequency();
      };

      auto lower = first + detail::to_signed<S>(first->node_size());
      auto upper = find_node_if(lower, last, has_higher_freq);

      // re-sort after creating a new internal node
      std::rotate(first, lower, upper);
    }
  }

  constexpr auto construct_table() -> void
  {
    using size_type = decltype(table_.size());

    if (table_.empty()) {
      return;
    }

    if (table_.size() == size_type{1}) {
      using namespace huffman::literals;
      static_cast<code&>(table_.front()) = 0_c;
      return;
    }

    auto reversed = std::views::reverse(table_);

    std::ranges::sort(reversed);

    // precondition
    assert(
        std::ranges::unique(
            reversed, {}, [](const auto& elem) { return elem.symbol; })
            .empty() and
        "a `table` cannot contain duplicate symbols");

    const auto frequencies = std::views::transform(
        reversed, [](const auto& elem) { return elem.frequency(); });
    [[maybe_unused]]
    const auto total_freq =
        std::accumulate(std::cbegin(frequencies), std::cend(frequencies), 0UZ);

    encode_symbols();

    // postcondition
    assert(total_freq == reversed.front().frequency());
  }

  constexpr auto set_skip_fields() -> void
  {
    using skip_type = decltype(std::declval<node_type>().skip());

    const node_type* prev{};

    for (auto& elem : std::views::reverse(table_)) {
      const auto s =
          skip_type{1} +
          (prev and (elem.bitsize() == prev->bitsize())
               ? prev->skip()
               : skip_type{});

      elem.set_skip(s);

      prev = &elem;
    }
  }

  /// Update table code values to DEFLATE canonical form
  ///
  /// The Huffman codes used for each alphabet in the "deflate" format have two
  /// additional rules:
  /// * All codes of a given bit length have lexicographically consecutive
  ///   values, in the same order as the symbols they represent;
  /// * Shorter codes lexicographically precede longer codes.
  ///
  /// @see section 3.2.2 https://datatracker.ietf.org/doc/html/rfc1951
  ///
  /// @note This method is called in all constructors except for the
  ///     table-contents constructor.
  ///
  constexpr auto canonicalize() & -> table&
  {
    using value_type = decltype(std::declval<code>().value());

    // set lexicographical order
    std::ranges::sort(  //
        table_,         //
        [](const auto& x, const auto& y) {
          return std::pair{x.bitsize(), std::ref(x.symbol)} <
                 std::pair{y.bitsize(), std::ref(y.symbol)};
        });

    // used to determine initial value of next_code[bits]
    // calculated in step 2
    auto base_code = value_type{};

    // used in determining consecutive code values in step 3
    auto next_code = code{};

    // clang-format off
    for (auto& elem : table_) {
      assert(next_code.bitsize() <= elem.bitsize());

      next_code = {
          elem.bitsize(),
          next_code.bitsize() == elem.bitsize()
              ? next_code.value() + value_type{1}                     // 3) next_code[len]++;
              : base_code <<= (elem.bitsize() - next_code.bitsize())  // 2) next_code[bits] = code; code = (...) << 1;
      };

      static_cast<code&>(elem) = next_code;                           // 3) tree[n].Code = next_code[len];

      ++base_code;                                                    // 2) (code + bl_count[bits-1])
    }
    // clang-format on

    set_skip_fields();

    return *this;
  }

public:
  /// Symbol type
  ///
  using symbol_type = Symbol;

  /// Code point type
  ///
  using encoding_type = encoding<symbol_type>;

  /// Const iterator type
  ///
  using const_iterator = detail::element_base_iterator<
      typename detail::table_storage<node_type, Extent>::const_iterator,
      encoding<Symbol>>;

  /// Constructs an empty table
  ///
  table() = default;

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
  constexpr table(const R& frequencies, std::optional<symbol_type> eot)
      : table_{detail::frequency_tag{}, frequencies, eot}
  {
    construct_table();
    canonicalize();
  }

  template <std::ranges::sized_range R>
    requires std::convertible_to<
        std::ranges::range_value_t<R>,
        std::tuple<symbol_type, std::size_t>>
  constexpr explicit table(const R& frequencies) : table{frequencies, {}}
  {}

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
  constexpr explicit table(const R& data, std::optional<symbol_type> eot)
      : table_{detail::data_tag{}, data, eot}
  {
    construct_table();
    canonicalize();
  }

  template <std::ranges::input_range R>
    requires std::convertible_to<std::ranges::range_reference_t<R>, symbol_type>
  constexpr explicit table(const R& data) : table{data, {}}
  {}

  /// @}

  /// Constructs a `table` from the given code-symbol mapping contents
  /// @tparam R sized-range of code-symbol 2-tuples
  /// @pre all `code` and `symbol` values container in mapping are unique
  /// @pre `code` values are specified in DEFLATE canonical form
  /// @pre `code` and `symbol` values are provided in lexicographical order
  ///
  /// Construct a `table` with explicit contents.
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
      : table_{table_contents, map}
  {
    assert(
        std::ranges::is_sorted(
            map,
            [](const auto& x, const auto& y) {
              const auto x_value = std::get<code>(x).value();
              const auto y_value = std::get<code>(y).value();

              const auto x_bitsize = std::get<code>(x).bitsize();
              const auto y_bitsize = std::get<code>(y).bitsize();

              const auto x_symbol = std::get<symbol_type>(x);
              const auto y_symbol = std::get<symbol_type>(y);

              return (x_value < y_value) and
                     ((x_bitsize < y_bitsize) or
                      ((x_bitsize == y_bitsize) and (x_symbol < y_symbol)));
            }) and
        "table contents are not provided in DEFLATE canonical form");
    set_skip_fields();
  }

  template <std::size_t N>
  constexpr table(
      table_contents_tag, const c_array<std::pair<code, symbol_type>, N>& map)
      : table{table_contents, std::ranges::ref_view{map}}
  {}

  /// @}

  /// Constructs a `table` from a symbol-bitsize mapping
  /// @tparam R sized-range of symbol-bitsize tuple-likes
  /// @param rng range of symbol-bitsize tuple-likes
  /// @pre all symbols are unique
  /// @pre the number of symbols with the same bitsize does not exceed the
  ///     available number of prefix free codes with that bitsize
  ///
  /// @{

  template <std::ranges::random_access_range R>
    requires std::convertible_to<
        std::ranges::range_reference_t<R>,
        std::tuple<symbol_span<symbol_type>, std::uint8_t>>
  constexpr table(symbol_bitsize_tag, const R& map)
      : table{table_contents,
              detail::flattened_symbol_bitsize_view{std::views::all(map)}}
  {
    canonicalize();
  }

  template <std::size_t N>
  constexpr table(
      symbol_bitsize_tag,
      const c_array<std::pair<symbol_span<symbol_type>, std::uint8_t>, N>& map)
      : table{symbol_bitsize, std::ranges::ref_view{map}}
  {}

  /// @}

  /// Returns an iterator to the first `encoding`
  ///
  /// @note
  /// * All codes of a given bit length have lexicographically consecutive
  ///   values, in the same order as the symbols they represent;
  /// * Shorter codes lexicographically precede longer codes.
  ///
  [[nodiscard]]
  constexpr auto begin() const -> const_iterator
  {
    return const_iterator{table_.begin()};
  }

  /// Returns an iterator past the last `encoding`
  /// @copydetail begin()
  ///
  [[nodiscard]]
  constexpr auto end() const -> const_iterator
  {
    return const_iterator{table_.end()};
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
    using D = std::iter_difference_t<const_iterator>;

    while (pos != end()) {
      if (pos->bitsize() > c.bitsize()) {
        break;
      }

      const auto skip = pos.base()->skip();

      if (pos->bitsize() == c.bitsize()) {
        assert(pos->value() <= c.value());

        if (const auto dist = c.value() - pos->value(); dist < skip) {
          return R{std::in_place, pos + static_cast<D>(dist)};
        }
      }

      assert(skip <= std::size_t{std::numeric_limits<D>::max()});
      pos += static_cast<D>(skip);
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

template <class S, class I, std::size_t N>
  requires (not detail::is_specialization_of_v<S, symbol_span>)
table(symbol_bitsize_tag, const c_array<std::pair<S, I>, N>&) -> table<S, N>;

template <class R>
  requires (
      detail::tuple_size_v<std::ranges::range_value_t<R>>() == 2 and
      detail::is_specialization_of_v<
          std::tuple_element_t<0, std::ranges::range_value_t<R>>,
          symbol_span>)
table(symbol_bitsize_tag, const R&) -> table<
    typename detail::tuple_arg_t<0, R>::symbol_type,
    detail::tuple_size_v<R>()>;

}  // namespace starflate::huffman
