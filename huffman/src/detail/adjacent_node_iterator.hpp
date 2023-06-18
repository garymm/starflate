#pragma once

#include "huffman/src/detail/iterator_interface.hpp"

#include <iterator>

namespace gpu_deflate::huffman::detail {

/// Iterator for a contiguous range of `table_node` values
///
/// Allows iteration over a contiguous range of `table_node`, using the value's
/// `node_size` in order to advance to the next adjacent node.
///
/// Given a contiguous container of nodes and an `adjacent_node_iterator` `i`,
/// `++i` returns an iterator to the next node, using `node_size()` to determine
/// if `i` is an internal node or a leaf node. If `i` is an internal node, (i.e.
/// `i` represents a node with children), `++i` skips the appropriate number of
/// elements in the associated container.
///
/// | freq: 3 | freq: 1 | freq: 1 | freq: 4 | freq: 2 |
/// | ns:   3 | ns:   1 | ns:   1 | ns:   2 | ns:   1 |
/// ^                             ^
/// i                             |
///                               |
/// ++i --------------------------+
///
template <class Node>
class adjacent_node_iterator
    : public iterator_interface<adjacent_node_iterator<Node>>
{
  Node* base_{};

  friend struct iterator_interface<adjacent_node_iterator>;

  constexpr auto preinc() -> adjacent_node_iterator&
  {
    base_ += base_->node_size();
    return *this;
  }

public:
  using base_iterator = Node*;

  using iterator_category = std::forward_iterator_tag;
  using value_type = Node;
  using reference = Node&;
  using pointer = Node*;
  using difference_type = std::ptrdiff_t;

  adjacent_node_iterator() = default;
  constexpr explicit adjacent_node_iterator(base_iterator it) : base_{it} {}

  constexpr auto base() const noexcept -> base_iterator { return base_; }

  constexpr auto operator*() const -> reference { return *base_; }

  friend constexpr auto
  operator<=>(adjacent_node_iterator, adjacent_node_iterator) = default;
};

}  // namespace gpu_deflate::huffman::detail
