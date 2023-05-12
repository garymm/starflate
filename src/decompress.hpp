#pragma once

#include <cstddef>
#include <memory>
#include <expected>
#include <span>
#include <vector>

namespace gpu_deflate {

// error code enum
enum class Error : std::uint8_t {
    Error,
};

// Inspired by https://docs.python.org/3/library/zlib.html#zlib.decompress
template <std::size_t N, class ByteAllocator = std::allocator<std::byte>>
std::expected<std::vector<std::byte>, Error> decompress(std::span<const std::byte, N> compressed, ByteAllocator alloc = {})
{
    (void) compressed;
    auto decompressed = std::vector<std::byte, ByteAllocator>(alloc);
    return decompressed;
}

}
