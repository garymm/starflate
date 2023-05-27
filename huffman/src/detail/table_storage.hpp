#pragma once

#include "huffman/src/detail/static_vector.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <optional>
#include <span>
#include <type_traits>
#include <vector>

namespace gpu_deflate::huffman::detail {

struct frequency_tag
{
  explicit frequency_tag() = default;
};
struct data_tag
{
  explicit data_tag() = default;
};

template <class IntrusiveNode, std::size_t Extent>
using table_storage_base_t = std::conditional_t<
    Extent == std::dynamic_extent,
    std::vector<IntrusiveNode>,
    static_vector<IntrusiveNode, Extent>>;

template <class IntrusiveNode, std::size_t Extent>
class table_storage : table_storage_base_t<IntrusiveNode, Extent>
{
public:
  using base_type = table_storage_base_t<IntrusiveNode, Extent>;
  using symbol_type = typename IntrusiveNode::symbol_type;

  template <class R>
  constexpr table_storage(
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
  constexpr table_storage(
      data_tag, const R& data, std::optional<symbol_type> eot)
      : base_type{}
  {
    if (eot) {
      base_type::emplace_back(*eot, 1UZ);
    }

    for (auto s : data) {
      assert(s != eot);
      assert(s != eot and "`eot` cannot be a symbol in `data``");

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

}  // namespace gpu_deflate::huffman::detail
