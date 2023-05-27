#pragma once

#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <ranges>
#include <utility>

namespace gpu_deflate::huffman {

/// A Huffman code
///
struct code
{
  std::uint8_t bitsize{};
  std::size_t value{};

  [[nodiscard]]
  constexpr auto bit_view() const
  {
    return std::views::iota(0UZ, std::size_t{bitsize}) | std::views::reverse |
           std::views::transform([value = this->value](auto n) {
             return (value & 1UZ << n) ? '1' : '0';
           });
  }

  friend auto operator<<(std::ostream& os, const code& c) -> std::ostream&
  {
    for (auto bit : c.bit_view()) {
      os << bit;
    }

    return os;
  }

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
