#pragma once

#include <concepts>
#include <iterator>

namespace gpu_deflate::huffman::detail {

// TODO: remove clang-format off after this is merged:
// https://github.com/llvm/llvm-project/issues/59412
// clang-format off

template <class D>
concept _forward_basis = requires (D i, typename D::difference_type n) {
  { *i } -> std::same_as<typename D::reference>;
  { i.preinc() } -> std::same_as<D&>;
};

template <class D>
concept _random_access_basis = requires (D i, typename D::difference_type n) {
  { *i } -> std::same_as<typename D::reference>;
  { i += n } -> std::same_as<D&>;
};

// clang-format on

/// CRTP helper class used to synthesize operations for a random access iterator
/// @tparam D derived iterator type
///
/// A simplified implementation of `iterator_interface`. This CRTP helper class
/// synthesizes iterator operations given a basis set of operations. The
/// synthesized operations are determined by the basis set of operations.
///
/// If `D` defines the following operations:
/// * operator*() const -> reference
/// * preinc() -> D&
/// and the following static member typedefs
/// * pointer
/// * reference
/// * difference_type
/// Then `iterator_interface` will synthesize operations allowing `D` to model
/// `std::forward_iterator.`
///
/// If `D` defines the following operations:
/// * operator*() const -> reference
/// * operator+=(difference_type) -> D&
/// and the following static member typedefs
/// * pointer
/// * reference
/// * difference_type
///
/// Then `iterator_interface` will synthesize operations allowing `D` to model
/// `std::random_access_iterator.` Note that `D` must also define the following
/// operations to model `std::random_access_iterator`, as these operations
/// cannot be synthesized:
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
    requires (_forward_basis<I> or _random_access_basis<I>)
  constexpr auto operator->() const -> typename I::pointer
  {
    return &*static_cast<const I&>(*this);
  }

  template <class I = D>
    requires _forward_basis<I> or _random_access_basis<I>
  constexpr auto operator++() -> I&
  {
    if constexpr (_random_access_basis<I>) {
      return static_cast<I&>(*this) += typename I::difference_type{1};
    } else if constexpr (_forward_basis<I>) {
      return static_cast<I&>(*this).preinc();
    }
  }

  template <class I = D>
    requires _forward_basis<I> or _random_access_basis<I>
  constexpr auto operator++(int) -> I
  {
    auto& self = static_cast<I&>(*this);

    auto tmp = self;
    ++self;
    return tmp;
  }

  template <class I = D>
    requires _random_access_basis<I>
  constexpr auto operator--() -> I&
  {
    return *this -= typename I::difference_type{1};
  }

  template <class I = D>
    requires _random_access_basis<I>
  constexpr auto operator--(int) -> I
  {
    auto& self = static_cast<I&>(*this);

    auto tmp = self;
    --self;
    return tmp;
  }

  template <class I = D>
    requires _random_access_basis<I>
  constexpr auto operator-=(typename I::difference_type n) -> I&
  {
    return static_cast<I&>(*this) += -n;
  }

  template <class I = D>
    requires _random_access_basis<I>
  constexpr auto
  operator[](typename I::difference_type n) const -> typename I::reference
  {
    return *(*this + n);
  }

  template <std::same_as<D> I>
    requires _random_access_basis<I>
  friend constexpr auto operator+(I i, typename I::difference_type n) -> I
  {
    return i += n;
  }

  template <std::same_as<D> I>
    requires _random_access_basis<I>
  friend constexpr auto operator+(typename I::difference_type n, I i) -> I
  {
    return i + n;
  }

  template <std::same_as<D> I>
    requires _random_access_basis<I>
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

}  // namespace gpu_deflate::huffman::detail
