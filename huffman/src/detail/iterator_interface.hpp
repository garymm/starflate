#pragma once

#include <concepts>
#include <iterator>

namespace starflate::huffman::detail {

/// CRTP helper class used to synthesize operations for a random access iterator
/// @tparam D derived iterator type
///
/// A simplified implementation of `iterator_interface`. This CRTP helper class
/// synthesizes iterator operations given a basis set of operations. This type
/// currently requires `D` to implement the following operations:
/// * operator*() const -> reference
/// * operator+=(difference_type) -> D&
/// and the following static member typedefs
/// * pointer
/// * reference
/// * difference_type
///
/// Note that `D` must also define the following operations to model
/// `std::random_access_iterator`
/// * D()
/// * operator-(const D&, const D&) -> difference_type
/// * operator<=>(const D&, const D&)
/// and static member typedefs
/// * iterator_category
/// * value_type
///
template <class D>
struct iterator_interface
{
  template <class I = D>
  constexpr auto operator->() const -> typename I::pointer
  {
    return &*static_cast<const I&>(*this);
  }

  template <class I = D>
  constexpr auto operator++() -> I&
  {
    return static_cast<I&>(*this) += typename I::difference_type{1};
  }

  template <class I = D>
  constexpr auto operator++(int) -> I
  {
    auto tmp = *this;
    ++*this;
    return tmp;
  }

  template <class I = D>
  constexpr auto operator--() -> I&
  {
    return *this -= typename I::difference_type{1};
  }

  template <class I = D>
  constexpr auto operator--(int) -> I
  {
    auto tmp = *this;
    --*this;
    return tmp;
  }

  template <class I = D>
  constexpr auto operator-=(typename I::difference_type n) -> I&
  {
    return static_cast<I&>(*this) += -n;
  }

  template <class I = D>
  constexpr auto
  operator[](typename I::difference_type n) const -> typename I::reference
  {
    return *(static_cast<const I&>(*this) + n);
  }

  template <std::same_as<D> I>
  friend constexpr auto operator+(I i, typename I::difference_type n) -> I
  {
    return i += n;
  }

  template <std::same_as<D> I>
  friend constexpr auto operator+(typename I::difference_type n, I i) -> I
  {
    return i + n;
  }

  template <std::same_as<D> I>
  friend constexpr auto operator-(I i, typename I::difference_type n) -> I
  {
    return i + -n;
  }

  /// Default three-way comparison
  ///
  // This must be defined to allow three-way comparison for derived class `I` to
  // be defaulted.
  friend constexpr auto
  operator<=>(const iterator_interface&, const iterator_interface&) = default;
};

}  // namespace starflate::huffman::detail
