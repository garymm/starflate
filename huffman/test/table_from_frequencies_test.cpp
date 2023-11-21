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

class Country
{
  std::array<char, 2> code_{};

public:
  Country() = default;

  // allow implicit conversions to simplify construction of test data
  // NOLINTNEXTLINE(google-explicit-constructor)
  Country(const char (&code)[3]) : code_{code[0], code[1]} {}

  friend auto operator<<(std::ostream& os, Country c) -> std::ostream&
  {
    const auto has_value = c.code_[0] != char{} and c.code_[1] != char{};

    os << std::string_view{c.code_.begin(), has_value ? 2UZ : 0UZ};
    return os;
  }

  friend constexpr auto operator<=>(Country lhs, Country rhs)
  {
    if (const auto cmp = lhs.code_[0] <=> rhs.code_[0]; cmp != 0) {
      return cmp;
    }
    return lhs.code_[1] <=> rhs.code_[1];
  }

  friend constexpr auto operator==(Country lhs, Country rhs)
  {
    return lhs <=> rhs == 0;
  }
};

auto main() -> int
{
  using ::boost::ut::aborts;
  using ::boost::ut::expect;
  using ::boost::ut::test;

  namespace huffman = ::starflate::huffman;

  test("code table is printable") = [] {
    const auto frequencies = std::vector<std::pair<char, std::size_t>>{
        {'e', 100}, {'n', 20}, {'x', 1}, {'i', 40}, {'q', 3}};

    constexpr auto eot = char{4};

    constexpr auto table =
        "Bits\tCode\tValue\tSymbol\n"
        "1\t0\t0\t`e`\n"
        "2\t10\t2\t`i`\n"
        "3\t110\t6\t`n`\n"
        "4\t1110\t14\t`q`\n"
        "5\t11110\t30\t`\4`\n"
        "5\t11111\t31\t`x`\n";

    auto ss = std::stringstream{};
    ss << huffman::table{frequencies, eot};

    expect(table == ss.str()) << ss.str();
  };

  test("code tables allow user defined types as symbols") = [] {
    const auto frequencies = std::vector<std::pair<Country, std::size_t>>{
        {"FR", 100}, {"UK", 20}, {"BE", 1}, {"IT", 40}, {"DE", 3}};

    const auto table = huffman::table{frequencies};

    using namespace huffman::literals;
    using encoding = huffman::encoding<Country>;

    expect(encoding{"FR", 0_c} == table.begin()[0]);
    expect(encoding{"IT", 10_c} == table.begin()[1]);
    expect(encoding{"UK", 110_c} == table.begin()[2]);
    expect(encoding{"BE", 1110_c} == table.begin()[3]);
    expect(encoding{"DE", 1111_c} == table.begin()[4]);
  };

  test("code table can be statically sized") = [] {
    const auto frequencies = std::vector<std::pair<char, std::size_t>>{
        {'e', 100}, {'n', 20}, {'x', 1}, {'i', 40}, {'q', 3}};

    constexpr auto eot = char{4};

    const auto t1 = huffman::table<char, 6>{frequencies, eot};
    const auto t2 = huffman::table<char, std::dynamic_extent>{frequencies, eot};

    expect(std::ranges::equal(t1, t2));
  };

  test("code table can deduce static extent") = [] {
    const auto frequencies = std::array<std::pair<char, std::size_t>, 5>{
        {{'e', 100}, {'n', 20}, {'x', 1}, {'i', 40}, {'q', 3}}};

    constexpr auto eot = char{4};

    const auto t1 = huffman::table<char, 6>{frequencies, eot};
    const auto t2 = huffman::table{frequencies, eot};

    expect(std::ranges::equal(t1, t2));
    static_assert(std::same_as<decltype(t1), decltype(t2)>);
  };

  test("code table rejects duplicate symbols") = [] {
    expect(aborts([] {
      huffman::table{
          std::vector<std::pair<char, std::size_t>>{{'e', 100}, {'e', 10}}};
    }));
  };

  test("code table can deduce symbol type and static extent") = [] {
    const auto frequencies = std::array<std::pair<char, std::size_t>, 5>{
        {{'e', 100}, {'n', 20}, {'x', 1}, {'i', 40}, {'q', 3}}};

    [[maybe_unused]] const auto t1 = huffman::table{frequencies};
  };

  test("code table can contain excess capacity with static extent") = [] {
    const auto frequencies = std::array<std::pair<char, std::size_t>, 5>{
        {{'e', 100}, {'n', 20}, {'x', 1}, {'i', 40}, {'q', 3}}};

    const auto t1 = huffman::table<char, 256>{frequencies};
    const auto t2 = huffman::table{frequencies};

    expect(sizeof(t1) > sizeof(t2));
    expect(std::ranges::equal(t1, t2));
  };

  test("code table constructible in constant expression context") = [] {
    static constexpr auto frequencies =  // clang-format off
        std::array<std::pair<char, std::size_t>, 5>{{
          {'e', 100},
          {'n', 20},
          {'x', 1},
          {'i', 40},
          {'q', 3}}};
    // clang-format on

    static constexpr auto table = huffman::table{frequencies};

    using namespace huffman::literals;
    using huffman::encoding;

    static_assert(encoding{'e', 0_c} == table.begin()[0]);
    static_assert(encoding{'i', 10_c} == table.begin()[1]);
    static_assert(encoding{'n', 110_c} == table.begin()[2]);
    static_assert(encoding{'q', 1110_c} == table.begin()[3]);
    static_assert(encoding{'x', 1111_c} == table.begin()[4]);
  };

  test("code table constructible in constant expression context with deduced "
       "frequencies type") = [] {
    static constexpr auto frequencies = std::array<
        std::pair<char, std::size_t>,
        5>{{{'e', 100}, {'n', 20}, {'x', 1}, {'i', 40}, {'q', 3}}};

    static constexpr auto t1 = huffman::table{frequencies};

    static constexpr auto t2 =  // clang-format off
      huffman::table{{
        std::pair{'e', 100},
                 {'n', 20},
                 {'x', 1},
                 {'i', 40},
                 {'q', 3},
      }};
    // clang-format on

    expect(std::ranges::equal(t1, t2));
  };
}

// NOLINTEND(readability-magic-numbers,google-build-using-namespace)
