#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <ostream>

namespace gpu_deflate::huffman {

/// A Huffman code
///
struct code
{
  std::uint8_t bitsize{};
  std::size_t value{};

  friend auto operator<<(std::ostream& os, const code& c) -> std::ostream&
  {
    auto bits = c.bitsize;

    while (bits != 0UZ) {
      --bits;
      os << (((1UZ << bits) & c.value) != 0UZ ? '0' : '1');
    }

    return os;
  }

  [[nodiscard]]
  friend auto
  operator<=>(const code&, const code&) = default;
};

}  // namespace gpu_deflate::huffman
