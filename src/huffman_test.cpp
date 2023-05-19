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

  friend auto operator<=>(Country, Country) = default;
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
        "5\t11111\t31\t`\4`\n"
        "5\t11110\t30\t`x`\n"
        "4\t1110\t14\t`q`\n"
        "3\t110\t6\t`n`\n"
        "2\t10\t2\t`i`\n"
        "1\t0\t0\t`e`\n";

    auto ss = std::stringstream{};
    ss << gpu_deflate::code_table{frequencies, eot};

    expect(table == ss.view());
  };

  test("code tables allow user defined types as symbols") = [] {
    const auto frequencies = std::vector<std::pair<Country, std::size_t>>{
        {"FR", 100}, {"UK", 20}, {"BE", 1}, {"IT", 40}, {"DE", 3}};

    const auto table = gpu_deflate::code_table{frequencies};

    using CodePoint = decltype(table)::code_point;

    expect(CodePoint{"FR", 1UZ, 0UZ} == table.begin()[0]);
    expect(CodePoint{"IT", 2UZ, 2UZ} == table.begin()[1]);
    expect(CodePoint{"UK", 3UZ, 6UZ} == table.begin()[2]);
    expect(CodePoint{"DE", 4UZ, 14UZ} == table.begin()[3]);
    expect(CodePoint{"BE", 5UZ, 30UZ} == table.begin()[4]);
  };
}
