#pragma once

#include <cassert>
#include <cstddef>
#include <span>
#include <vector>

namespace gpu_deflate {
namespace detail {

template <class intrusive_node, std::size_t Extent>
class huffman_storage : std::vector<intrusive_node>
{
  static_assert(Extent == std::dynamic_extent, "not yet implemented");

public:
  using base_type = std::vector<intrusive_node>;

  template <class R>
  huffman_storage(
      const R& frequencies, typename intrusive_node::symbol_type eot)
      : base_type{}
  {
    base_type::reserve(frequencies.size() + 1UZ);  // +1 for EOT
    base_type::emplace_back(eot, 1UZ);

    for (auto [symbol, freq] : frequencies) {
      assert(symbol != eot);
      assert(freq);

      base_type::emplace_back(symbol, freq);
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

}  // namespace detail
}  // namespace gpu_deflate
