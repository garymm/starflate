#pragma once

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <fstream>
#include <ios>
#include <iterator>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace starflate {
namespace test {

/// Container of bytes that allows `std::byte`-based traversal
/// @tparam Container underlying byte storage container
///
/// `byte_container` is a container type that allows iteration over a range of
/// `std::byte` values. Constructors handle conversion of input ranges of `char`
/// values to `std::byte` values.
///
template <std::ranges::contiguous_range Container>
  requires std::same_as<std::byte, std::ranges::range_value_t<Container>>
class byte_container
    : public std::ranges::view_interface<byte_container<Container>>
{
  Container storage_;

  static constexpr auto byte_view = std::views::transform([](auto c) {
    return static_cast<std::byte>(c);
  });

public:
  using container_type = Container;

  /// Construct a bit container from a file
  /// @tparam FsPath filename type
  /// @tparam Args args forwarded to construct container_type
  /// @param filename name of file to be opened
  /// @param container_args args forwarded to the underlying container
  /// constructor
  ///
  /// Opens a file specified by `filename` and reads its contents into the
  /// underlying container.
  ///
  /// @pre `container_type{std::forward<Args>(container_args)...}.empty()` is
  /// `true`
  ///
  /// @see https://en.cppreference.com/w/cpp/io/basic_ifstream/basic_ifstream
  ///
  template <class FsPath, class... Args>
    requires std::constructible_from<std::ifstream, FsPath&&> and
             std::constructible_from<Container, Args&&...>
  byte_container(FsPath&& filename, Args&&... container_args)
      : storage_{std::forward<Args>(container_args)...}
  {
    assert(
        storage_.empty() and
        "underlying container must be empty before "
        "reading input file");

    auto input = std::ifstream{
        filename,
        std::ios_base::in | std::ios_base::binary | std::ios_base::ate};
    if (not input.is_open()) {
      using namespace std::string_literals;
      throw std::runtime_error{"unable to open: "s + filename};
    }

    auto size = input.tellg();
    assert(size >= 0);
    storage_.reserve(static_cast<std::size_t>(size));
    input.seekg(0);

    const auto bytes =
        std::ranges::subrange{
            std::istreambuf_iterator<char>{input},
            std::istreambuf_iterator<char>{}} |
        byte_view;

    // GCC doesn't define append_range yet
    std::ranges::copy(bytes, std::back_inserter(storage_));
  }

  /// Construct from a string-like
  ///
  template <class... Args>
    requires std::constructible_from<Container, Args&&...>
  explicit byte_container(
      std::in_place_t, std::string_view s, Args&&... container_args)
      : storage_{std::forward<Args>(container_args)...}
  {
    assert(
        storage_.empty() and
        "underlying container must be empty before "
        "reading setting bytes");

    storage_.reserve(static_cast<std::size_t>(s.size()));

    std::ranges::copy(s | byte_view, std::back_inserter(storage_));
  }

  /// Access the underlying storage container
  ///
  /// {@

  [[nodiscard]]
  auto underlying() const& -> const container_type&
  {
    return storage_;
  }
  [[nodiscard]]
  auto underlying() && -> container_type&&
  {
    return std::move(storage_);
  }

  /// @}

  [[nodiscard]]
  auto begin() const -> decltype(storage_.begin())
  {
    return storage_.begin();
  }
  [[nodiscard]]
  auto end() const -> decltype(storage_.end())
  {
    return storage_.end();
  }
};

using byte_vector = byte_container<std::vector<std::byte>>;

}  // namespace test
}  // namespace starflate
