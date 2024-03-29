#pragma once

#include <array>
#include <concepts>
#include <cstddef>

namespace starflate::huffman {

/// Convenience alias for a C-style array
///
template <class T, std::size_t N>
using c_array = T[N];

/// Disambiguation tag to specify a table is constructed with a code-symbol
///    mapping
///
struct table_contents_tag
{
  explicit table_contents_tag() = default;
};
inline constexpr auto table_contents = table_contents_tag{};

/// Disambiguation tag to specify a table is constructed with a symbol-bitsize
///    mapping
///
struct symbol_bitsize_tag
{
  explicit symbol_bitsize_tag() = default;
};
inline constexpr auto symbol_bitsize = symbol_bitsize_tag{};

template <class... Ts>
constexpr auto byte_array(Ts... values)
{
  return std::array<std::byte, sizeof...(Ts)>{std::byte(values)...};
}

/// Specifies that a type that can be used as a symbol
///
template <class T>
concept symbol = std::regular<T> and std::totally_ordered<T>;

}  // namespace starflate::huffman
