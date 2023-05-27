#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <optional>
#include <span>
#include <stdexcept>
#include <vector>

namespace gpu_deflate::detail {

struct frequency_tag
{
  explicit frequency_tag() = default;
};
struct data_tag
{
  explicit data_tag() = default;
};

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

template <class Node, std::size_t Extent>
using huffman_storage_base_t = std::conditional_t<
    Extent == std::dynamic_extent,
    std::vector<Node>,
    static_vector<Node, Extent>>;

template <class Node, std::size_t Extent>
class huffman_storage : huffman_storage_base_t<Node, Extent>
{
public:
  using base_type = huffman_storage_base_t<Node, Extent>;
  using symbol_type = typename Node::symbol_type;

  template <class R>
  constexpr huffman_storage(
      frequency_tag, const R& frequencies, std::optional<symbol_type> eot)
      : base_type{}
  {
    base_type::reserve(frequencies.size() + std::size_t{eot.has_value()});
    if (eot) {
      base_type::emplace_back(*eot, 1UZ);
    }

    for (auto [symbol, freq] : frequencies) {
      assert(symbol != eot and "`eot` cannot be a symbol in `frequencies``");
      assert(freq and "the frequency for a symbol must be positive");

      base_type::emplace_back(symbol, freq);
    }
  }

  template <class R>
  constexpr huffman_storage(
      data_tag, const R& data, std::optional<symbol_type> eot)
      : base_type{}
  {
    if (eot) {
      base_type::emplace_back(*eot, 1UZ);
    }

    for (auto s : data) {
      assert(s != eot);

      const auto lower = std::ranges::lower_bound(*this, s, {}, [](auto& node) {
        return node.base().symbol;
      });

      if (lower != base_type::cend() and lower->base().symbol == s) {
        *lower = {s, lower->frequency() + 1UZ};
      } else {
        base_type::emplace(lower, s, 1UZ);
      }
    }
  }

  using base_type::begin;
  using base_type::cbegin;
  using base_type::cend;
  using base_type::data;
  using base_type::end;
  using base_type::front;
  using base_type::size;
};

}  // namespace gpu_deflate::detail
