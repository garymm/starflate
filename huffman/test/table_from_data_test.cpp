#include "huffman/huffman.hpp"

#include <boost/ut.hpp>

#include <array>
#include <concepts>
#include <cstddef>
#include <ranges>
#include <sstream>
#include <string_view>
#include <utility>
#include <vector>

// allow magic numbers and namespace directives in tests
// NOLINTBEGIN(readability-magic-numbers,google-build-using-namespace)

auto main() -> int
{
  using namespace ::boost::ut;

  namespace huffman = ::starflate::huffman;

  test("code table constructible from symbol sequence") = [] {
    const auto frequencies = std::vector<std::pair<char, std::size_t>>{
        {'e', 100}, {'n', 20}, {'x', 1}, {'i', 40}, {'q', 3}};

    const auto data = [&frequencies] {
      auto d = std::vector<char>{};

      for (auto [s, n] : frequencies) {
        d.insert(d.cend(), n, s);
      }

      return d;
    }();

    constexpr auto eot = char{4};

    const auto t1 = huffman::table{frequencies, eot};
    const auto t2 = huffman::table{data, eot};

    expect(std::ranges::equal(t1, t2));
  };

  test("code table constructible from symbol sequence in constant expression "
       "context") = [] {
    static constexpr auto frequencies = std::array<
        std::pair<char, std::size_t>,
        5>{{{'e', 100}, {'n', 20}, {'x', 1}, {'i', 40}, {'q', 3}}};

    constexpr auto count = std::accumulate(
        frequencies.cbegin(), frequencies.cend(), 0UZ, [](auto acc, auto arg) {
          return acc + std::get<std::size_t>(arg);
        });

    static constexpr auto data = [] {
      auto d = std::array<char, count>{};

      auto it = d.begin();
      for (auto [s, n] : frequencies) {
        it = std::fill_n(it, n, s);
      }

      return d;
    }();

    constexpr auto eot = char{4};

    static constexpr auto t1 = huffman::table{frequencies, eot};
    static constexpr auto t2 = huffman::table<char, 256>{data, eot};

    expect(constant<sizeof(t1) < sizeof(t2)>);
    expect(std::ranges::equal(t1, t2));
  };

  test("code table constructible from symbol sequence without eot") = [] {
    using namespace std::literals;

    constexpr auto data = "this is an example of a huffman tree"sv;

    const auto t1 = huffman::table{data};
    static constexpr auto t2 = huffman::table<char, 256>{data};

    expect(std::ranges::equal(t1, t2));
  };

  test("empty code table") = [] {
    const auto zero = huffman::table<char, 0>{};

    expect(zero.begin() == zero.end());
  };

  test("one symbol code table") = [] {
    using namespace std::literals;

    constexpr auto data = "a"sv;

    const auto one = huffman::table<char, 1>{data};

    expect('a' == one.begin()->symbol);
    expect(1 == one.begin()->bitsize());
  };
}

// NOLINTEND(readability-magic-numbers,google-build-using-namespace)
