#pragma once

#include "huffman/src/detail/iterator_interface.hpp"

#include <concepts>
#include <iterator>
#include <ranges>
#include <utility>

namespace gpu_deflate::huffman::detail {

/// A view of elements cast to a base class
/// @tparam V underlying view
/// @tparam B base class
///
template <std::ranges::random_access_range V, class B>
  requires std::ranges::view<V> and
           std::same_as<std::ranges::iterator_t<V>,
                        std::ranges::sentinel_t<V>> and
           std::derived_from<std::ranges::range_value_t<V>, B>
class base_view : public std::ranges::view_interface<base_view<V, B>>
{
  // This is largely adapted from `transform_view` (or other views), although we
  // apply some simplifications:
  // * V must model `random_access_range` instead of `input_range`
  // * sentinel_t<V> is the same as iterator_t<V>
  //
  // https://eel.is/c++draft/range.transform

  V base_{};

public:
  class iterator : public iterator_interface<iterator>
  {
    using base_iterator = std::ranges::iterator_t<V>;
    base_iterator base_{};

  public:
    using iterator_category =
        typename std::iterator_traits<base_iterator>::iterator_category;
    using value_type = std::ranges::range_value_t<V>;
    using reference = B&;
    using pointer = B*;
    using difference_type = std::ranges::range_difference_t<V>;

    iterator()
      requires std::default_initializable<base_iterator>
    = default;
    constexpr iterator(base_iterator current) : base_{std::move(current)} {}

    constexpr auto base() const& noexcept -> const base_iterator&
    {
      return base_;
    }
    constexpr auto base() && -> base_iterator { return std::move(base_); }

    constexpr auto operator*() const -> reference
    {
      return static_cast<reference>(*base_);
    }

    constexpr auto operator+=(difference_type n) -> iterator&
    {
      base_ += n;
      return *this;
    }

    friend constexpr auto
    operator-(const iterator& x, const iterator& y) -> difference_type
    {
      return x.base() - y.base();
    }

    friend constexpr auto
    operator<=>(const iterator&, const iterator&) = default;
  };

  base_view()
    requires std::default_initializable<V>
  = default;
  constexpr explicit base_view(V base) : base_{std::move(base)} {}

  constexpr auto base() const& -> V
    requires std::copy_constructible<V>
  {
    return base_;
  }
  constexpr auto base() && -> V { return std::move(base_); }

  constexpr auto begin() const -> iterator { return {base().begin()}; }
  constexpr auto end() const -> iterator { return {base().end()}; }
};

// Use of range_adaptor_clousure requires GCC 13 or LLVM > 16.0.0
// While this isn't necessary, it does allow a simpler declaration of
// `base_view_` type in the `table` class template.

// namespace views {
//
// template <class B>
// struct base_raco : std::ranges::range_adaptor_closure<base_raco<B>>
//{
//  template <std::ranges::forward_range V>
//  constexpr auto operator()(V v) const
//    -> base_view<V, B>
//  {
//    return base_view<V, B>{v}
//  }
//};
//
// template <class B>
// inline constexpr auto base = base_raco<B>{};
//
//}

}  // namespace gpu_deflate::huffman::detail
