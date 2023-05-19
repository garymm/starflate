#include <boost/ut.hpp>

auto main() -> int
{
  using namespace ::boost::ut;

  test("dummy test") = [] { expect(1_i == 1); };
}
