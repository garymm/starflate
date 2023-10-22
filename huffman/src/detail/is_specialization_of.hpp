#pragma once

#include <type_traits>

namespace starflate::huffman::detail {

/// Checks if a type is a specialization of a class template
/// @tparam T type
/// @tparam primary class template
///
/// @see https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p2098r1.pdf
///
/// @{

template <class T, template <class...> class primary>
struct is_specialization_of : std::false_type
{};

template <template <class...> class primary, class... Args>
struct is_specialization_of<primary<Args...>, primary> : std::true_type
{};

template <class T, template <class...> class primary>
inline constexpr auto is_specialization_of_v =
    is_specialization_of<T, primary>::value;

/// @}

}  // namespace starflate::huffman::detail
