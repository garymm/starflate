#include "huffman/src/bit.hpp"
#include <cstddef>
#include <iterator>


namespace gpu_deflate {
namespace huffman {
    class bit_span {

        const std::byte* data_;
        size_t bit_size_;

    public:

        class iterator {
            const bit_span* parent_;
            size_t offset_;

            public:
            using iterator_category = std::random_access_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = bit;
            using reference = bit;
            using pointer = void;


            iterator() = default;
            iterator(const bit_span& parent, size_t offset) : parent_(&parent), offset_(offset) {}
        };


        bit_span(const std::byte* data, size_t bit_size) : data_(data), bit_size_(bit_size) {}



        iterator begin() const;
        iterator end() const;



    };
}
}
