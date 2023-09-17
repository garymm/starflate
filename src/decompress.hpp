#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>
#include <span>
#include <vector>

namespace starflate {

// error code enum
enum class Error : std::uint8_t
{
  Error,
};

// Inspired by https://docs.python.org/3/library/zlib.html#zlib.decompress
template <std::size_t N, class ByteAllocator = std::allocator<std::byte>>
auto decompress(
    [[maybe_unused]] std::span<const std::byte, N> compressed,
    ByteAllocator alloc = {}) -> std::expected<std::vector<std::byte>, Error>
{
  auto decompressed = std::vector<std::byte, ByteAllocator>(alloc);
  return decompressed;
}

}  // namespace starflate
