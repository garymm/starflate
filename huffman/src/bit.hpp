#pragma once

#include <cassert>
#include <ostream>

namespace gpu_deflate::huffman {

/// A distinct type to represent a bit
///
/// A type representing a bit. It is used for strongly-typed bit operations with
/// `code` values.
///
class bit
{
  bool value_{};

public:
  /// Constructs a zero-bit
  ///
  bit() = default;

  /// Constructs a bit from an int
  /// @pre value == 1 or value == 0
  ///
  constexpr explicit bit(int value) : value_{value == 1}
  {
    assert(value == 1 or value == 0);
  }

  /// Constructs a bit from a char
  /// @pre value == '1' or value == '0'
  ///
  constexpr explicit bit(char value) : value_{value == '1'}
  {
    assert(value == '1' or value == '0');
  }

  /// Constructs a bit from a bool
  ///
  constexpr explicit bit(bool value) : value_{value} {}

  /// Obtains the representation as a `bool`
  ///
  constexpr explicit operator bool() const noexcept { return value_; }

  /// Obtains the representation as a `char`
  ///
  constexpr explicit operator char() const noexcept
  {
    return value_ ? '1' : '0';
  }

  /// Inserts a textual representation of `b` into `os`
  ///
  friend auto operator<<(std::ostream& os, bit b) -> std::ostream&
  {
    os << static_cast<char>(b);
    return os;
  }

  /// Compares two bits
  ///
  friend auto operator==(bit, bit) -> bool = default;
};

namespace literals {

/// Forms a `bit` literal
///
consteval auto operator""_b(unsigned long long int n) -> bit
{
  using I [[maybe_unused]] = unsigned long long int;
  assert(n == I{} or n == I{1});

  return bit{static_cast<int>(n)};
}

}  // namespace literals

}  // namespace gpu_deflate::huffman
