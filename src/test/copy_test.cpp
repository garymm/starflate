#include "src/copy.hpp"

#include <boost/ut.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <iterator>
#include <list>

auto main() -> int
{
  using ::boost::ut::aborts;
  using ::boost::ut::expect;
  using ::boost::ut::range_eq;
  using ::boost::ut::test;
  using std::ranges::subrange;

  test("copy with overlap, non-contiguous range") = [] {
    constexpr auto expected = std::array{1, 2, 3, 1, 2, 3, 1, 2};

    auto data = std::list{1, 2, 3, 0, 0, 0, 0, 0};

    const auto result =
        starflate::copy_n(data.begin(), 5, std::next(data.begin(), 3));

    expect(std::next(data.begin(), 5) == result.in);
    expect(data.end() == result.out);
    expect(range_eq(expected, data));
  };

  test("copy, adjacent ranges, contiguous range") = [] {
    constexpr auto expected = std::array{1, 2, 3, 4, 1, 2, 3, 4};

    auto data = std::array{1, 2, 3, 4, 0, 0, 0, 0};

    auto result = starflate::copy_n(data.begin(), 4, data.begin() + 4);

    expect(data.begin() + 4 == result.in);
    expect(data.end() == result.out);
    expect(range_eq(expected, data));
  };

  test("copy with overlap, contiguous range") = [] {
    constexpr auto expected = std::array{1, 2, 3, 1, 2, 3, 1, 2};

    auto data = std::array{1, 2, 3, 0, 0, 0, 0, 0};

    auto result = starflate::copy_n(data.begin(), 5, data.begin() + 3);

    expect(data.begin() + 5 == result.in);
    expect(data.end() == result.out);
    expect(range_eq(expected, data));
  };

  test("copy with overlap, contiguous, constexpr") = [] {
    constexpr auto expected = std::array{1, 2, 3, 1, 2, 3, 1, 2};

    constexpr auto data = [] {
      auto data = std::array{1, 2, 3, 0, 0, 0, 0, 0};

      auto result = starflate::copy_n(data.begin(), 5, data.begin() + 3);
      assert(result.out == data.cend());

      return data;
    }();

    expect(range_eq(expected, data));
  };

  test("different ranges without overlap") = [] {
    static constexpr auto expected = std::array{1, 2, 3, 4};

    constexpr auto actual = [] {
      auto buffer = std::array<int, 6>{};

      const auto dest = subrange{buffer};

      auto remaining = starflate::copy(expected, dest);
      assert(remaining.begin() == dest.begin() + expected.size());
      assert(remaining.size() == dest.size() - expected.size());

      return buffer;
    }();

    using std::views::take;
    expect(range_eq(expected, actual | take(4)));
  };

  test("same range without overlap") = [] {
    static constexpr auto expected = std::array{1, 2, 3, 1, 2, 3};

    constexpr auto actual = [] {
      auto buffer = std::array<int, 6>{1, 2, 3};

      const auto src = subrange{buffer.cbegin(), buffer.cbegin() + 3};
      const auto dest = subrange{buffer}.next(3);

      auto remaining = starflate::copy(src, dest);
      assert(remaining.empty());

      return buffer;
    }();

    expect(range_eq(expected, actual));
  };

  test("same range with overlap") = [] {
    static constexpr auto expected = std::array{1, 2, 1, 2, 1, 0};

    constexpr auto actual = [] {
      auto buffer = std::array<int, 6>{1, 2};

      const auto src = subrange{buffer.cbegin(), buffer.cbegin() + 3};
      const auto dest = subrange{buffer}.next(2);

      auto remaining = starflate::copy(src, dest);
      assert(remaining.size() == dest.size() - src.size());
      assert(remaining.begin() == buffer.cbegin() + 5);

      return buffer;
    }();

    expect(range_eq(expected, actual));
  };

  test("destination range too small") = [] {
    static constexpr auto expected = std::array<int, 4>{};

    expect(aborts([] {
      auto buffer = std::array<int, 3>{};
      starflate::copy(expected, subrange{buffer});
    }));
  };
}
