#include <cstdint>
#include <istream>
#include <ostream>

namespace starflate {
enum class GunzipError : std::uint8_t
{
  NoError,
  Error,
};
auto gunzip(std::istream& in, std::ostream& out) -> GunzipError;
}  // namespace starflate
