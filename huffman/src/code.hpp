#pragma once

#include "huffman/src/bit.hpp"

#include <cassert>
#include <climits>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <ranges>
#include <utility>

namespace gpu_deflate::huffman {

/// A Huffman code
///
class code
{
  std::uint8_t bitsize_{};
  std::size_t value_{};

public:
  /// Constructs an empty code
  ///
  code() = default;

  /// Construct a code from a bitsize and value
  /// @pre the position of the most significant bit of `value` <= `bitsize`
  ///
  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
  constexpr code(std::uint8_t bitsize, std::size_t value)
      : bitsize_{bitsize}, value_{value}
  {
    // NOLINTNEXTLINE(readability-magic-numbers)
    static_assert(CHAR_BIT == 8U, "everything assumes 8 bits per byte");

    // TODO: use std::countl_zero when available in GCC and Clang.
    // https://en.cppreference.com/w/cpp/numeric/countl_zero
    [[maybe_unused]] const auto msb =
        64UZ - (value ? std::size_t(__builtin_clzll(value)) : 64UZ);
    assert(msb <= std::size_t{bitsize} and "`value` exceeds `bitsize`");
  }

  /// Returns number of bits used to represent the code
  ///
  [[nodiscard]]
  constexpr auto bitsize() const
  {
    return bitsize_;
  }

  /// Returns the integral value of the code
  ///
  [[nodiscard]]
  constexpr auto value() const
  {
    return value_;
  }

  /// Returns a view of `*this` as a range of bits, from left to right
  ///
  [[nodiscard]]
  constexpr auto bit_view() const
  {
    return std::views::iota(0UZ, std::size_t{bitsize()}) | std::views::reverse |
           std::views::transform([value = value()](auto n) {
             return (value & 1UZ << n) ? bit{1} : bit{0};
           });
  }

  /// Left pad `c` with `b`
  ///
  friend constexpr auto operator>>(bit b, code& c) -> code&
  {
    if (b) {
      c.value_ += (1UZ << c.bitsize_);
    }

    ++c.bitsize_;
    return c;
  }
  friend constexpr auto operator>>(bit b, code&& c) -> code&&
  {
    b >> c;
    return std::move(c);
  }

  /// Right pad `c` with `b`
  ///
  friend constexpr auto operator<<(code& c, bit b) -> code&
  {
    c.value_ <<= 1U;
    c.value_ |= static_cast<size_t>(bool(b));
    ++c.bitsize_;
    return c;
  }
  friend constexpr auto operator<<(code&& c, bit b) -> code&&
  {
    c << b;
    return std::move(c);
  }

  /// Inserts a textual representation of `c` into `os`
  ///
  friend auto operator<<(std::ostream& os, const code& c) -> std::ostream&
  {
    for (auto bit : c.bit_view()) {
      os << bit;
    }

    return os;
  }

  /// Compares two codes
  ///
  [[nodiscard]]
  friend auto
  operator<=>(const code&, const code&) = default;
};

namespace detail {

inline constexpr auto bit_shift = [](char c, std::size_t n) {
  assert(c == '0' or c == '1');

  return ((c - '0') == 0 ? 0UZ : 1UZ) << n;
};

}

namespace literals {

/// Forms a `code` literal
///
template <char... Bits>
consteval auto operator""_c() -> code
{
  constexpr auto N = sizeof...(Bits);

  using ::gpu_deflate::huffman::detail::bit_shift;

  return {N, []<std::size_t... Is>(std::index_sequence<Is...>) {
            return (bit_shift(Bits, N - 1 - Is) | ...);
          }(std::make_index_sequence<N>{})};
}

}  // namespace literals
}  // namespace gpu_deflate::huffman
