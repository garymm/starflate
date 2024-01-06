#pragma once
#include "huffman/src/bit.hpp"
#include "huffman/src/detail/iterator_interface.hpp"

#include <bit>
#include <bitset>
#include <cassert>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <limits>
#include <ranges>

namespace starflate::huffman {
/// A non-owning span of bits. Allows for iteration over the individual bits.
class bit_span : public std::ranges::view_interface<bit_span>
{

public:
  // TODO: make private
  const std::byte* data_{nullptr};
  std::size_t bit_size_{};
  std::uint8_t bit_offset_{};  // always less than CHAR_BIT
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
        : parent_{&parent}, offset_{offset}
    {
      assert(offset_ <= std::numeric_limits<difference_type>::max());
    }

    constexpr auto operator*() const -> reference
    {
      const auto byte = std::bitset<CHAR_BIT>{
          // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
          static_cast<unsigned long long>(parent_->data_[offset_ / CHAR_BIT])};

      return bit{byte[offset_ % CHAR_BIT]};
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

  /// Constructs an empty bit_span
  ///
  bit_span() = default;

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

  template <std::integral T>
  constexpr auto pop() -> T
  {
    assert(
        bit_size_ >= sizeof(T) * CHAR_BIT and
        "bit_span has insufficient "
        "remaining bits to pop");
    assert(bit_offset_ == 0 and "bit_span must be byte aligned to pop");
    T res;
    std::memcpy(&res, data_, sizeof(T));
    std::advance(data_, sizeof(T));
    bit_size_ -= sizeof(T) * CHAR_BIT;
    if constexpr (std::endian::native == std::endian::big) {
      res = std::byteswap(res);
    }
    return res;
  }

  constexpr auto pop_8() -> std::uint8_t { return pop<std::uint8_t>(); }

  constexpr auto pop_16() -> std::uint16_t { return pop<std::uint16_t>(); }

  /// Removes n bits from the beginning of this and returns them.
  ///
  /// @pre this contains at least n bits.
  ///
  constexpr auto pop_n(std::uint8_t n) -> std::uint16_t
  {
    assert(n <= 16);
    assert(n <= bit_size_);
    auto iter = begin();
    std::uint16_t res{};
    for (std::uint8_t i{}; i < n; i++) {
      res |= static_cast<std::uint16_t>(
          static_cast<std::uint16_t>(static_cast<bool>(*iter)) << i);
      iter += 1;
    }
    consume(n);  // invalidates iter, so must come after the loop
    return res;
  }

  /// Consumes the given number of bits. Advances the start of the view.
  ///
  /// @pre n <= std::ranges::size(*this)
  ///
  constexpr auto consume(std::size_t n) & -> bit_span&
  {
    assert(n <= bit_size_);  // pre
    bit_size_ -= n;

    const auto distance = bit_offset_ + n;
    std::advance(data_, distance / CHAR_BIT);
    bit_offset_ = static_cast<std::uint8_t>(distance % CHAR_BIT);

    assert(bit_offset_ < CHAR_BIT);  // post
    return *this;
  }
  constexpr auto consume(std::size_t n) && -> bit_span&&
  {
    consume(n);
    return std::move(*this);
  }

  /// Consumes bits until the start is aligned to a byte boundary.
  constexpr auto consume_to_byte_boundary() -> void
  {
    if (bit_offset_) {
      consume(CHAR_BIT - bit_offset_);
    }
  }

  /// Returns a pointer to the underlying data.
  /// @pre *this aligned to a byte boundary.
  [[nodiscard]]
  constexpr auto byte_data() const -> const std::byte*
  {
    assert(bit_offset_ == 0 and "bit_span must be byte aligned to access data");
    return data_;
  }
};
}  // namespace starflate::huffman
