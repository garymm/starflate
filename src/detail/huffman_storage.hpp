#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <span>
#include <vector>

namespace gpu_deflate::detail {

struct frequency_tag
{
  explicit frequency_tag() = default;
};
struct data_tag
{
  explicit data_tag() = default;
};

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
  huffman_storage(frequency_tag, const R& frequencies, symbol_type eot)
      : base_type{}
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
  constexpr huffman_storage(
      frequency_tag, const R& frequencies, symbol_type eot)
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

  template <class R>
    requires (Extent == std::dynamic_extent)
  huffman_storage(data_tag, const R& data, symbol_type eot) : base_type{}
  {
    base_type::emplace_back(eot, 1UZ);

    for (auto s : data) {
      assert(s != eot);

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

}  // namespace gpu_deflate::detail
