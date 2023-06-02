#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <utility>

namespace gpu_deflate::huffman::detail {

/// A simplified implementation of `static_vector`
///
/// Provides a vector-like container with fixed capacity. Unlike the
/// `static_vector` proposed in P0843, this type will default construct all
/// elements in storage instead of using placement new.
///
template <class T, std::size_t Capacity>
class static_vector : std::array<T, Capacity>
{
  using base_type = std::array<T, Capacity>;
  typename base_type::size_type size_{};

public:
  using iterator = typename base_type::iterator;
  using const_iterator = typename base_type::const_iterator;
  using reference = typename base_type::reference;
  using size_type = typename base_type::size_type;

  static_vector() = default;

  using base_type::begin;
  using base_type::cbegin;
  using base_type::data;
  using base_type::front;

  constexpr auto reserve(size_type new_cap) -> void
  {
    if (new_cap > Capacity) {
      throw std::length_error{"`static_vector` capacity exceeded."};
    }
  }

  constexpr auto resize(size_type new_cap) -> void
  {
    reserve(new_cap);

    if (size() < new_cap) {
      const auto first = begin() + size();
      std::for_each(first, first + (new_cap - size()), [](auto& elem) {
        elem = {};
      });
    }

    size_ = new_cap;
  }

  template <class... Args>
  constexpr auto emplace_back(Args&&... args) -> reference
  {
    reserve(size() + 1UZ);

    ++size_;
    return *std::prev(end()) = T{std::forward<Args>(args)...};
  }

  template <class... Args>
  constexpr auto emplace(const_iterator pos, Args&&... args) -> iterator
  {
    const auto first = begin() + (pos - begin());
    const auto mid = end();

    emplace_back(std::forward<Args>(args)...);

    const auto last = end();

    std::rotate(first, mid, last);
    return first;
  }

  constexpr auto size() const noexcept -> size_type { return size_; }

  constexpr auto end() noexcept -> iterator { return begin() + size(); }
  constexpr auto end() const noexcept -> const_iterator
  {
    return begin() + size();
  }
  constexpr auto cend() const noexcept -> const_iterator { return end(); }
};

}  // namespace gpu_deflate::huffman::detail
