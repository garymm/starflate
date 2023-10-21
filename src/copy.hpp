#pragma once

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <ranges>
#include <type_traits>

namespace starflate {

/// Copies a number of elements to a new location
/// @tparam I input_iterator of the source range
/// @tparam O output_iterator of the destination range
/// @param first beginning of the range to copy from
/// @param n number of element to copy
/// @param result beginning of the destination range
///
/// Copies exactly `n` values from the range beginning at first to the range
/// beginning at `result` by performing the equivalent of `*(result + i) =
/// *(first + i)` for each integer in [`0`, `n`).
///
/// @return `std::ranges::in_out_result` that contains an `std::input_iterator`
///     iterator equal to `std::ranges::next(first, n)` and a
///     `std::weakly_incrementable` iterator equal to `ranges::next(result, n)`.
///
/// @pre `n >= 0`
/// @pre destination range does not overlap left side of source range
///
/// @note Unlike `std::copy_n` and `std::ranges::copy_n`, `starflate::copy_n`
///     specifically handles a destination range overlapping the right side of a
///     source range.
///
/// @note This is implemented as a global function object.
///
/// @see https://en.cppreference.com/w/cpp/algorithm/copy_n
///
inline constexpr class
{
  template <class I, class D, class O>
  static constexpr auto impl(std::true_type, I first, D n, O result)
      -> std::ranges::in_out_result<I, O>
  {
    if (std::is_constant_evaluated()) {
      return impl(std::false_type{}, first, n, result);
    }

    // FIXME this is potential UB
    // https://stackoverflow.com/questions/56036264/what-is-the-rationale-of-making-subtraction-of-two-pointers-not-related-to-the-s
    // https://stackoverflow.com/questions/47616508/what-is-the-rationale-for-limitations-on-pointer-arithmetic-or-comparison
    const auto dist = result - first;

    while (n != D{}) {
      const auto m = std::min(dist, n);
      std::memcpy(result, first, static_cast<std::size_t>(m) * sizeof(*first));
      first += m;
      result += m;
      n -= m;
    }

    return {first, result};
  }

  template <class I, class D, class O>
  static constexpr auto impl(std::false_type, I first, D n, O result)
      -> std::ranges::in_out_result<I, O>
  {
    assert(n >= D{} and "`n` must be non-negative");

    while (n-- != D{}) {
      *result++ = *first++;
    }

    return {first, result};
  }

public:
  template <std::input_iterator I, std::weakly_incrementable O>
    requires std::indirectly_copyable<I, O>
  constexpr auto
  operator()(I first, std::iter_difference_t<I> n, O result) const
      -> std::ranges::in_out_result<I, O>
  {
    using try_bulk_copy = std::bool_constant<
        std::contiguous_iterator<I> and                                 //
        std::contiguous_iterator<O> and                                 //
        std::is_same_v<std::iter_value_t<I>, std::iter_value_t<O>> and  //
        std::is_trivially_copyable_v<std::iter_value_t<I>>>;

    return impl(try_bulk_copy{}, first, n, result);
  }
} copy_n{};

/// Copies a source range into the beginning of a destination range
/// @tparam R source range
/// @tparam O iterator type of destination range
/// @param source range to copy from
/// @param dest range to copy to
///
/// Copies bytes from the source range to the destination range.
///
/// @return `std::ranges::subrange` containing the unwritten subrange of
///     `dest`
///
/// @pre `std::ranges::size(source) <= dest.size()` and destination range does
///     not overlap left side of source range.
///
/// @note `starflate::copy_n` specifically handles a destination range
///     overlapping the right side of a source range.
///
/// @note This is implemented as a global function object.
///
inline constexpr class
{
public:
  template <std::ranges::sized_range R, std::random_access_iterator O>
    requires std::indirectly_copyable<std::ranges::iterator_t<R>, O>
  constexpr auto
  operator()(const R& source, std::ranges::subrange<O> dest) const
      -> std::ranges::subrange<O>
  {
    const auto n = std::ranges::ssize(source);
    assert(
        n <= std::ranges::ssize(dest) and
        "destination range is smaller "
        "than source range");

    const auto result = copy_n(std::ranges::cbegin(source), n, dest.begin());

    assert(result.in == std::ranges::cend(source));
    return dest.next(n);
  }
} copy{};

}  // namespace starflate
