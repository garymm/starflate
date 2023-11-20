#include "huffman/src/utility.hpp"
#include "src/decompress.hpp"
#include "tools/cpp/runfiles/runfiles.h"

#include <boost/ut.hpp>

#include <fstream>
#include <iterator>
#include <memory>
#include <vector>

template <class... Ts>
constexpr auto byte_vector(Ts... values)
{
  return std::vector<std::byte>{std::byte(values)...};
}

auto read_runfile(const char* argv0, const std::string& path)
    -> std::vector<std::byte>
{
  using ::bazel::tools::cpp::runfiles::Runfiles;
  std::string error;
  std::unique_ptr<Runfiles> runfiles(Runfiles::Create(argv0, &error));
  ::boost::ut::expect(::boost::ut::fatal(runfiles != nullptr)) << error;

  const std::string abs_path{runfiles->Rlocation(path)};

  std::ifstream file{abs_path, std::ios::binary};
  ::boost::ut::expect(::boost::ut::fatal(file.is_open()))
      << "failed to open " << path;
  std::vector<char> chars(
      (std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

  const std::vector<char> char_vector(
      (std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

  // The standard library really doesn't want me to read std::byte from a file,
  // so reinterpret chars as bytes.
  // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
  return {
      reinterpret_cast<std::byte*>(chars.data()),
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      reinterpret_cast<std::byte*>(chars.data() + chars.size())};
  // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
}

auto main(int, char* argv[]) -> int
{
  using ::boost::ut::eq;
  using ::boost::ut::expect;
  using ::boost::ut::fatal;
  using ::boost::ut::test;
  using namespace starflate;

  test("read_header") = [] -> void {
    huffman::bit_span empty{nullptr, 0, 0};
    expect(detail::read_header(empty).error() ==
           DecompressError::InvalidBlockHeader);

    constexpr auto bad_block_type = huffman::byte_array(0b111);
    huffman::bit_span bad_block_type_span{bad_block_type};
    expect(detail::read_header(bad_block_type_span).error() ==
           DecompressError::InvalidBlockHeader);

    constexpr auto fixed = huffman::byte_array(0b010);
    huffman::bit_span fixed_span{fixed};
    auto header = detail::read_header(fixed_span);
    expect(header.has_value())
        << "got error: " << static_cast<int>(header.error());
    expect(not header->final);
    expect(header->type == detail::BlockType::FixedHuffman)
        << "got type: " << static_cast<int>(header->type);

    constexpr auto no_compression = huffman::byte_array(0b001);
    huffman::bit_span no_compression_span{no_compression};
    header = detail::read_header(no_compression_span);
    expect(header.has_value())
        << "got error: " << static_cast<int>(header.error());
    expect(header->final);
    expect(header->type == detail::BlockType::NoCompression)
        << "got type: " << static_cast<int>(header->type);
  };

  test("no compression") = [] {
    constexpr auto compressed = huffman::byte_array(
        0b001,
        5,
        0,  // len = 5
        ~5,
        ~0,  // nlen = 5
        'h',
        'e',
        'l',
        'l',
        'o');

    const auto expected = byte_vector('h', 'e', 'l', 'l', 'o');

    const auto actual = decompress(compressed);
    expect(fatal(actual.has_value()))
        << "got error code: " << static_cast<std::int32_t>(actual.error());
    expect(fatal(actual->size() == expected.size()));
    expect(*actual == expected);
  };

  test("fixed huffman") = [argv] {
    const std::vector<std::byte> input_bytes =
        read_runfile(*argv, "starflate/src/test/starfleet.html.fixed");
    huffman::bit_span input_bits(input_bytes);

    const auto header = detail::read_header(input_bits);
    expect(header.has_value())
        << "got error: " << static_cast<int>(header.error());
    expect(header->type == detail::BlockType::FixedHuffman);
  };

  test("dynamic huffman") = [argv] {
    const std::vector<std::byte> input_bytes =
        read_runfile(*argv, "starflate/src/test/starfleet.html.dynamic");
    huffman::bit_span input_bits(input_bytes);

    const auto header = detail::read_header(input_bits);
    expect(header.has_value())
        << "got error: " << static_cast<int>(header.error());
    expect(header->type == detail::BlockType::DynamicHuffman);
  };
};
