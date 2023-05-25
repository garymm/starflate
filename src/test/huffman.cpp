#include "src/huffman.hpp"

#include <boost/ut.hpp>

#include <array>
#include <concepts>
#include <cstddef>
#include <ranges>
#include <sstream>
#include <string_view>
#include <utility>
#include <vector>

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
  using namespace ::boost::ut;

  test("code table is printable") = [] {
    const auto frequencies = std::vector<std::pair<char, std::size_t>>{
        {'e', 100}, {'n', 20}, {'x', 1}, {'i', 40}, {'q', 3}};

    constexpr auto eot = char{4};

    constexpr auto table =
        "Bits\tCode\tValue\tSymbol\n"
        "5\t11111\t0\t`\4`\n"
        "5\t11110\t1\t`x`\n"
        "4\t1110\t1\t`q`\n"
        "3\t110\t1\t`n`\n"
        "2\t10\t1\t`i`\n"
        "1\t0\t1\t`e`\n";

    auto ss = std::stringstream{};
    ss << gpu_deflate::code_table{frequencies, eot};

    expect(table == ss.str());
  };

  test("code tables allow user defined types as symbols") = [] {
    const auto frequencies = std::vector<std::pair<Country, std::size_t>>{
        {"FR", 100}, {"UK", 20}, {"BE", 1}, {"IT", 40}, {"DE", 3}};

    const auto table = gpu_deflate::code_table{frequencies};

    using CodePoint = decltype(table)::code_point_type;

    expect(CodePoint{"FR", 1UZ, 1UZ} == table.begin()[5]);
    expect(CodePoint{"IT", 2UZ, 1UZ} == table.begin()[4]);
    expect(CodePoint{"UK", 3UZ, 1UZ} == table.begin()[3]);
    expect(CodePoint{"DE", 4UZ, 1UZ} == table.begin()[2]);
    expect(CodePoint{"BE", 5UZ, 1UZ} == table.begin()[1]);
  };

  test("code table can be statically sized") = [] {
    const auto frequencies = std::vector<std::pair<char, std::size_t>>{
        {'e', 100}, {'n', 20}, {'x', 1}, {'i', 40}, {'q', 3}};

    constexpr auto eot = char{4};

    const auto t1 = gpu_deflate::code_table<char, 6>{frequencies, eot};
    const auto t2 =
        gpu_deflate::code_table<char, std::dynamic_extent>{frequencies, eot};

    expect(std::ranges::equal(t1, t2));
  };

  test("code table can deduce static extent") = [] {
    const auto frequencies = std::array<std::pair<char, std::size_t>, 5>{
        {{'e', 100}, {'n', 20}, {'x', 1}, {'i', 40}, {'q', 3}}};

    constexpr auto eot = char{4};

    const auto t1 = gpu_deflate::code_table<char, 6>{frequencies, eot};
    const auto t2 = gpu_deflate::code_table{frequencies, eot};

    expect(std::ranges::equal(t1, t2));
    static_assert(std::same_as<decltype(t1), decltype(t2)>);
  };

  test("code table can deduce symbol type and static extent") = [] {
    const auto frequencies = std::array<std::pair<char, std::size_t>, 5>{
        {{'e', 100}, {'n', 20}, {'x', 1}, {'i', 40}, {'q', 3}}};

    const auto t1 = gpu_deflate::code_table{frequencies};
    (void)t1;
  };

  test("code table rejects duplicate symbols") = [] {
    expect(aborts([] {
      gpu_deflate::code_table{
          std::vector<std::pair<char, std::size_t>>{{'e', 100}, {'e', 10}}};
    }));
  };

  test("code table constructible in constant expression context") = [] {
    static constexpr auto frequencies = std::array<
        std::pair<char, std::size_t>,
        5>{{{'e', 100}, {'n', 20}, {'x', 1}, {'i', 40}, {'q', 3}}};

    static constexpr auto table = gpu_deflate::code_table{frequencies};

    using CodePoint = decltype(table)::code_point_type;

    static_assert(CodePoint{'e', 1UZ, 1UZ} == table.begin()[5]);
    static_assert(CodePoint{'i', 2UZ, 1UZ} == table.begin()[4]);
    static_assert(CodePoint{'n', 3UZ, 1UZ} == table.begin()[3]);
    static_assert(CodePoint{'q', 4UZ, 1UZ} == table.begin()[2]);
    static_assert(CodePoint{'x', 5UZ, 1UZ} == table.begin()[1]);
  };
}
