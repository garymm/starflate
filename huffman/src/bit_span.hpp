#pragma once
#include "huffman/src/bit.hpp"
#include "huffman/src/detail/iterator_interface.hpp"

#include <bitset>
#include <cassert>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <ranges>

namespace starflate::huffman {
/// A non-owning span of bits. Allows for iteration over the individual bits.
class bit_span : public std::ranges::view_interface<bit_span>
{
  const std::byte* data_;
  std::size_t bit_size_;
  std::uint8_t bit_offset_;  // always less than CHAR_BIT

public:
  /// An iterator over the bits in a bit_span.
  class iterator : public detail::iterator_interface<iterator>
  {
  private:
    const bit_span* parent_;
    std::size_t offset_;

  public:
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::random_access_iterator_tag;
    using value_type = bit;
    using reference = bit;
    using pointer = void;

    iterator() = default;
    constexpr iterator(const bit_span& parent, std::size_t offset)
        : parent_(&parent), offset_(offset)
    {
      assert(offset_ <= std::numeric_limits<difference_type>::max());
    }

    constexpr auto operator*() const -> reference
    {
      const auto byte = std::bitset<CHAR_BIT>{
          // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
          static_cast<unsigned long long>(parent_->data_[offset_ / CHAR_BIT])};

      return bit{byte[CHAR_BIT - 1 - (offset_ % CHAR_BIT)]};
    }

    constexpr auto operator+=(difference_type n) -> iterator&
    {
      const auto newOffset = static_cast<difference_type>(offset_) + n;
      assert(newOffset >= 0);
      offset_ = static_cast<std::size_t>(newOffset);
      return *this;
    }

    friend constexpr auto
    operator-(const iterator& lhs, const iterator& rhs) -> difference_type
    {
      assert(lhs.parent_ == rhs.parent_);

      return static_cast<difference_type>(lhs.offset_) -
             static_cast<difference_type>(rhs.offset_);
    }

    friend constexpr auto
    operator<=>(const iterator&, const iterator&) = default;
  };

  /// Constructs a bit_span from the given data.
  ///
  /// @param data a pointer to the first byte of the data.
  /// @param bit_size the number of bits in the data.
  /// @param bit_offset bit offset of data, allowing a non-byte aligned range
  ///
  /// @pre offset < CHAR_BIT
  ///
  // NOLINTBEGIN(bugprone-easily-swappable-parameters)
  constexpr bit_span(
      const std::byte* data, std::size_t bit_size, std::uint8_t bit_offset = {})
      : data_{data}, bit_size_{bit_size}, bit_offset_{bit_offset}
  // NOLINTEND(bugprone-easily-swappable-parameters)
  {
    assert(
        bit_offset < CHAR_BIT and
        "bit offset exceeds number of bits in a "
        "byte");
  }

  template <std::ranges::contiguous_range R>
    requires std::ranges::borrowed_range<R>
  // TODO: remove cppcoreguidelines-pro-type-member-init once
  // https://reviews.llvm.org/D157367 in our toolchain.
  // NOLINTBEGIN(cppcoreguidelines-pro-type-member-init)
  // lack of forward is intentional as we constrain on borrowed_range
  // NOLINTNEXTLINE(bugprone-forwarding-reference-overload,cppcoreguidelines-missing-std-forward)
  constexpr bit_span(R&& r)
      : bit_span(std::ranges::data(r), std::ranges::size(r) * CHAR_BIT)
  {}
  // NOLINTEND(cppcoreguidelines-pro-type-member-init)

  [[nodiscard]]
  constexpr auto begin() const -> iterator
  {
    return iterator{*this, bit_offset_};
  };
  [[nodiscard]]
  constexpr auto end() const -> iterator
  {
    return iterator{*this, bit_offset_ + bit_size_};
  };
};
}  // namespace starflate::huffman
