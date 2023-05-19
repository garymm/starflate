#pragma once

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
#include <queue>
#include <ranges>
#include <type_traits>
#include <utility>
#include <vector>

namespace gpu_deflate {
namespace detail {

constexpr auto pop(auto& heap)
{
  auto r = std::move(heap.top());
  heap.pop();
  return r;
}

template <class T>
concept Consume = not std::is_reference_v<T>;

constexpr auto join(Consume auto&& container, Consume auto&& range)
{
  container.insert(
      std::cend(container),
      std::make_move_iterator(std::begin(range)),
      std::make_move_iterator(std::end(range)));

  return std::move(container);
}
}  // namespace detail

/// Huffman code table
/// @tparam Symbol symbol type
///
/// Determines Huffman code for a collection of symbols
///
template <std::regular Symbol>
class code_table
{
public:
  /// Symbol type
  ///
  using symbol_type = Symbol;

  /// Internal code table element type
  ///
  struct code_point
  {
    symbol_type symbol{};
    std::uint8_t bitsize{};
    std::size_t value{};

    constexpr auto join_from_left() -> void
    {
      ++bitsize;  // left pad with a 0
    }

    constexpr auto join_from_right() -> void
    {
      // left pad with a 1
      value += (1UZ << bitsize++);
    }

    struct code_type
    {
      std::uint8_t bitsize{};
      std::size_t value{};

      friend auto
      operator<<(std::ostream& os, const code_type& c) -> std::ostream&
      {
        auto bits = c.bitsize;

        while (bits != 0U) {
          --bits;
          os << ((1UZ << bits) & c.value ? '1' : '0');
        }

        return os;
      }
    };

    constexpr auto code() const -> code_type { return {bitsize, value}; }

    friend auto
    operator<<(std::ostream& os, const code_point& point) -> std::ostream&
    {
      os << +point.bitsize        //
         << "\t" << point.code()  //
         << "\t" << point.value   //
         << "\t`" << point.symbol << '`';

      return os;
    }

    friend auto operator<=>(const code_point&, const code_point&) = default;
  };

private:
  std::vector<code_point> table_{};

  struct tree
  {
    std::size_t frequency{};
    std::vector<code_point> children{};

    friend constexpr auto
    operator<=>(const tree& lhs, const tree& rhs) noexcept -> std::weak_ordering
    {
      return lhs.frequency <=> rhs.frequency;
    }

    friend auto operator|(tree&& lhs, tree&& rhs) -> tree
    {
      using std::ranges::for_each;
      for_each(lhs.children, &code_point::join_from_left);
      for_each(rhs.children, &code_point::join_from_right);

      return {lhs.frequency + rhs.frequency,
              detail::join(std::move(lhs.children), std::move(rhs.children))};
    }
  };

  template <class R>
  static auto impl(const R& frequencies, symbol_type eot)
  {
    const auto total_freq = std::accumulate(
        std::cbegin(frequencies),
        std::cend(frequencies),
        1UZ,  // for EOT which we add later.
        [](auto acc, auto kv) { return acc + kv.second; });

    auto vec = std::vector<tree>{};
    vec.reserve(frequencies.size() + 1UZ);  // 1 for EOT

    auto heap = std::priority_queue{std::greater<>{}, std::move(vec)};
    heap.emplace(1UZ, std::vector{code_point{eot}});

    for (auto [symbol, freq] : frequencies) {
      assert(symbol != eot);
      assert(freq);

      heap.emplace(freq, std::vector{code_point{symbol}});
    }

    using detail::pop;
    while (heap.size() > 1UZ) {
      heap.push(pop(heap) | pop(heap));
    }

    auto [f, code_table] = pop(heap);
    assert(total_freq == f);
    return code_table;
  }

public:
  /// Constructs a `code_table` from a symbol-frequency mapping
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
  explicit code_table(const R& frequencies) : code_table{frequencies, {}}
  {}

  template <std::ranges::sized_range R>
    requires std::convertible_to<
        std::ranges::range_value_t<R>,
        std::tuple<symbol_type, std::size_t>>
  code_table(const R& frequencies, symbol_type eot)
      : table_{impl(frequencies, eot)}
  {}

  /// @}

  /// Return an iterator to the beginning/end
  ///
  /// @{

  auto begin() const { return table_.begin(); }

  auto end() const { return table_.end(); }

  /// @}

  friend auto
  operator<<(std::ostream& os, const code_table& table) -> std::ostream&
  {
    os << "Bits\tCode\tValue\tSymbol\n";
    for (const auto& entry : table | std::views::reverse) {
      os << entry << '\n';
    }
    return os;
  }
};

template <class R>
  requires (
      std::tuple_size_v<std::ranges::range_value_t<R>> == 2 and
      std::regular<std::tuple_element_t<0, std::ranges::range_value_t<R>>> and
      std::convertible_to<
          std::tuple_element_t<1, std::ranges::range_value_t<R>>,
          std::size_t>)
code_table(const R&)
    -> code_table<std::tuple_element_t<0, std::ranges::range_value_t<R>>>;

}  // namespace gpu_deflate
