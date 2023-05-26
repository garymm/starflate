#include "huffman.hpp"

#include <boost/ut.hpp>

#include <array>
#include <cstddef>
#include <sstream>
#include <string_view>
#include <utility>
#include <vector>

class Country
{
  std::array<char, 2> code_{};

public:
  Country() = default;
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
        "1\t1\t1\t`e`\n"
        "2\t01\t1\t`i`\n"
        "3\t001\t1\t`n`\n"
        "4\t0001\t1\t`q`\n"
        "5\t00001\t1\t`x`\n"
        "5\t00000\t0\t`\4`\n";

    auto ss = std::stringstream{};
    ss << gpu_deflate::code_table{frequencies, eot};

    expect(table == ss.str());
  };

  test("code tables allow user defined types as symbols") = [] {
    const auto frequencies = std::vector<std::pair<Country, std::size_t>>{
        {"FR", 100}, {"UK", 20}, {"BE", 1}, {"IT", 40}, {"DE", 3}};

    const auto table = gpu_deflate::code_table{frequencies};

    using CodePoint = decltype(table)::code_point;

    expect(CodePoint{"FR", 1UZ, 1UZ} == table.begin()[5]);
    expect(CodePoint{"IT", 2UZ, 1UZ} == table.begin()[4]);
    expect(CodePoint{"UK", 3UZ, 1UZ} == table.begin()[3]);
    expect(CodePoint{"DE", 4UZ, 1UZ} == table.begin()[2]);
    expect(CodePoint{"BE", 5UZ, 1UZ} == table.begin()[1]);
  };
}
