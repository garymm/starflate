#pragma once

#include "huffman/src/detail/iterator_interface.hpp"

#include <concepts>
#include <iterator>
#include <type_traits>
#include <utility>

namespace starflate::huffman::detail {

/// Iterator adaptor for traversal of elements as a base type
/// @tparam I iterator
/// @tparam B base class
///
/// element_base_iterator is an iterator adaptor that returns a reference to an
/// element's base type instead of a reference to an element itself.
///
template <std::random_access_iterator I, class B>
  requires std::is_object_v<B> and
           std::derived_from<std::iter_value_t<I>, B> and
           // prevents iterators that return proxy objects.
           // In order for static_cast<B> to be safe, I has to return ref
           // to the actual object.
           std::is_same_v<
               B,
               std::remove_cvref_t<
                   std::common_reference_t<B&, std::iter_reference_t<I>>>>
class element_base_iterator
    : public iterator_interface<element_base_iterator<I, B>>
{
  I base_iterator_{};

public:
  using base_iterator_type = I;
  using element_base_type = B;

  using iterator_category = std::random_access_iterator_tag;
  using value_type = element_base_type;
  using reference =
      std::common_reference_t<element_base_type&, std::iter_reference_t<I>>;
  using pointer = std::remove_reference_t<reference>*;
  using difference_type = std::iter_difference_t<base_iterator_type>;

  element_base_iterator()
    requires std::default_initializable<base_iterator_type>
  = default;
  constexpr explicit element_base_iterator(base_iterator_type iter)
      : base_iterator_{std::move(iter)}
  {}

  constexpr auto base() const& noexcept -> const base_iterator_type&
  {
    return base_iterator_;
  }
  constexpr auto base() && noexcept -> base_iterator_type
  {
    return std::move(base_iterator_);
  }

  constexpr auto operator*() const -> reference
  {
    return static_cast<reference>(*base_iterator_);
  }

  constexpr auto operator+=(difference_type n) -> element_base_iterator&
  {
    base_iterator_ += n;
    return *this;
  }

  friend constexpr auto
  operator-(const element_base_iterator& x, const element_base_iterator& y)
      -> difference_type
  {
    return x.base() - y.base();
  }

  friend constexpr auto operator<=>(
      const element_base_iterator&, const element_base_iterator&) = default;
};

}  // namespace starflate::huffman::detail
