#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <span>
#include <vector>

namespace gpu_deflate::detail {

template <class Node, std::size_t Extent>
using huffman_storage_base_t = std::conditional_t<
    Extent == std::dynamic_extent,
    std::vector<Node>,
    std::array<Node, Extent>>;

template <class Node, std::size_t Extent>
class huffman_storage : huffman_storage_base_t<Node, Extent>
{
public:
  using base_type = huffman_storage_base_t<Node, Extent>;
  using symbol_type = typename Node::symbol_type;

  template <class R>
    requires (Extent == std::dynamic_extent)
  huffman_storage(const R& frequencies, symbol_type eot) : base_type{}
  {
    base_type::reserve(frequencies.size() + 1UZ);  // +1 for EOT
    base_type::emplace_back(eot, 1UZ);

    for (auto [symbol, freq] : frequencies) {
      assert(symbol != eot and "`eot` cannot be a symbol in `frequencies``");
      assert(freq and "the frequency for a symbol must be positive");

      base_type::emplace_back(symbol, freq);
    }
  }

  template <class R>
    requires (Extent != std::dynamic_extent)
  huffman_storage(const R& frequencies, symbol_type eot)
      : base_type{{{eot, 1UZ}}}
  {
    assert(frequencies.size() + 1UZ == Extent);

    auto& base = static_cast<base_type&>(*this);
    auto i = std::size_t{};

    for (auto [symbol, freq] : frequencies) {
      assert(symbol != eot);
      assert(freq);

      base[++i] = {symbol, freq};
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
