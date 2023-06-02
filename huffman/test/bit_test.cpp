#include "huffman/huffman.hpp"

#include <boost/ut.hpp>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <sstream>

auto main() -> int
{
  using ::boost::ut::aborts;
  using ::boost::ut::expect;
  using ::boost::ut::test;

  namespace huffman = ::gpu_deflate::huffman;
  using namespace huffman::literals;

  test("bit is truthy") = [] {
    expect(not huffman::bit{0});
    expect(bool(huffman::bit{1}));
  };

  test("bit is constructible with literal") = [] {
    expect(huffman::bit{0} == 0_b);
    expect(huffman::bit{1} == 1_b);
  };

  test("bit is constructible from char") = [] {
    expect(huffman::bit{'0'} == 0_b);
    expect(huffman::bit{'1'} == 1_b);
  };

  test("bit is constructible from bool") = [] {
    expect(huffman::bit{false} == 0_b);
    expect(huffman::bit{true} == 1_b);
  };

  test("bit is ostreamable") = [] {
    {
      auto ss = std::stringstream{};
      ss << 0_b;
      expect(ss.str() == "0");
    }

    {
      auto ss = std::stringstream{};
      ss << 1_b;
      expect(ss.str() == "1");
    }
  };

  test("bit aborts if constructed with out of range value") = [] {
    expect(aborts([] { huffman::bit{-1}; }));
    expect(aborts([] { huffman::bit{2}; }));
    expect(aborts([] { huffman::bit{'2'}; }));
  };
}
