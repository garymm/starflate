#pragma once

#include <include/boost/ut.hpp>

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <ostream>
#include <ranges>
#include <type_traits>

namespace starflate::test::detail {

template <class Printer>
auto& print(Printer& p, std::byte b)
{
  using U = std::uint8_t;

  const auto value = static_cast<std::uint8_t>(b);

  for (auto mask = U{0x80U}; mask != U{}; mask >>= 1U) {
    p << (static_cast<bool>(value & mask) ? '1' : '0');
  }

  return p;
}

}  // namespace starflate::test::detail

template <
    class Ostream,
    class = std::enable_if_t<
        std::derived_from<std::remove_cvref_t<Ostream>, std::ostream>>>
auto operator<<(Ostream&& os, std::byte b)
    -> decltype(os << char{}, std::forward<Ostream>(os))
{
  ::starflate::test::detail::print(os, b);
  return std::forward<Ostream>(os);
}

namespace boost::ut {

auto& operator<<(printer& p, std::byte b)
{
  return ::starflate::test::detail::print(p, b);
}

namespace detail {

template <class TLhs, class TRhs>
struct range_eq_ : op
{
  constexpr range_eq_(const TLhs& lhs = {}, const TRhs& rhs = {})
      : lhs_{lhs},
        rhs_{rhs},
        error_{[&] -> std::remove_const_t<decltype(error_)> {
          if constexpr (
              type_traits::is_range_v<TLhs> and type_traits::is_range_v<TRhs>) {
            using std::ranges::cend;

            auto count = unsigned{};

            const auto result = std::ranges::mismatch(
                lhs_, rhs_, [&count](const auto& x, const auto& y) {
                  using std::operator==;
                  ++count;
                  return x == y;
                });

            if (result.in1 == cend(lhs_) and result.in2 == cend(rhs_)) {
              return {};
            }

            if (result.in1 != cend(lhs_) and result.in2 != cend(rhs_)) {
              return {{count - 1U, result.in1, result.in2}};
            }

            return {{count, result.in1, result.in2}};
          }
        }()}
  {}

  [[nodiscard]]
  constexpr
  operator bool() const
  {
    return not error_;
  }
  [[nodiscard]]
  constexpr auto& lhs() const
  {
    return lhs_;
  }
  [[nodiscard]]
  constexpr auto& rhs() const
  {
    return rhs_;
  }
  [[nodiscard]]
  constexpr auto error() const
  {
    return *error_;
  }

  const TLhs lhs_{};
  const TRhs rhs_{};
  const std::optional<std::tuple<
      unsigned,
      std::ranges::iterator_t<const TLhs>,
      std::ranges::iterator_t<const TRhs>>>
      error_{};

  friend auto& operator<<(printer& p, const range_eq_& op)
  {
    static constexpr auto colors_ = colors{};
    static constexpr auto color = [](bool cond) {
      return cond ? colors_.pass : colors_.fail;
    };

    p << color(op) << "ranges_eq(" << reflection::decay_type_name<TLhs>()
      << ", " << reflection::decay_type_name<TRhs>() << ")";

    if (not op) {
      const auto [index, in1, in2] = op.error();

      p << "\n at element " << index << ": ";

      if (in1 != op.lhs().end() and in2 != op.rhs().end()) {
        p << "[ " << *in1 << " == " << *in2 << " ]";
      } else if (in1 == op.lhs().end()) {
        p << "lhs has fewer elements than rhs";
      } else {
        p << "lhs has more elements than rhs";
      }
    }

    return (p << colors_.none);
  }
};

}  // namespace detail

template <class TLhs, class TRhs>
[[nodiscard]]
constexpr auto range_eq(const TLhs& lhs, const TRhs& rhs)
{
  return detail::range_eq_{lhs, rhs};
}

}  // namespace boost::ut
