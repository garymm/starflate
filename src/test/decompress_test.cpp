#include "huffman/huffman.hpp"
#include "huffman/src/utility.hpp"
#include "src/decompress.hpp"
#include "tools/cpp/runfiles/runfiles.h"

#include <boost/ut.hpp>

#include <array>
#include <fstream>
#include <iterator>
#include <memory>
#include <vector>

namespace {
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
  if (not file.is_open()) {
    // ::boost::ut::fatal swallows log messages, so log before.
    ::boost::ut::log("failed to open file: " + abs_path);
    ::boost::ut::expect(::boost::ut::fatal(false));
  }

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
}  // namespace

auto main(int, char* argv[]) -> int
{
  using ::boost::ut::eq;
  using ::boost::ut::expect;
  using ::boost::ut::test;
  using namespace starflate;

  test("read_header") = [] -> void {
    huffman::bit_span empty{nullptr, 0, 0};
    expect(detail::read_header(empty).error() ==
           DecompressStatus::InvalidBlockHeader);

    constexpr auto bad_block_type = huffman::byte_array(0b111);
    huffman::bit_span bad_block_type_span{bad_block_type};
    expect(detail::read_header(bad_block_type_span).error() ==
           DecompressStatus::InvalidBlockHeader);

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

  test("decompress invalid header") = [] -> void {
    const auto status =
        decompress(std::span<const std::byte>{}, std::span<std::byte>{});
    expect(status == DecompressStatus::InvalidBlockHeader);
  };

  test("no compression") = [] {
    constexpr auto compressed = huffman::byte_array(
        0b000,  // no compression, not final
        4,
        0,  // len = 4
        ~4,
        ~0,  // nlen = 4
        'r',
        'o',
        's',
        'e',
        0b001,  // no compression, final
        3,
        0,  // len = 3
        ~3,
        ~0,  // nlen = 3
        'b',
        'u',
        'd');
    const std::span<const std::byte> src{compressed};

    constexpr auto expected =
        huffman::byte_array('r', 'o', 's', 'e', 'b', 'u', 'd');

    std::array<std::byte, expected.size()> dst_array{};
    const std::span<std::byte> dst_too_small{
        dst_array.data(), dst_array.size() - 1};
    const auto status_dst_too_small = decompress(src, dst_too_small);
    expect(status_dst_too_small == DecompressStatus::DstTooSmall);

    const std::span<std::byte> dst{dst_array};
    const auto status_src_too_small = decompress(src.subspan(0, 5), dst);
    expect(status_src_too_small == DecompressStatus::SrcTooSmall);

    const auto status = decompress(src, dst);
    expect(status == DecompressStatus::Success);
    expect(std::ranges::equal(dst, expected));
  };

  test("fixed huffman") = [argv] {
    const std::vector<std::byte> input_bytes =
        read_runfile(*argv, "starflate/src/test/starfleet.html.fixed");
    huffman::bit_span input_bits(input_bytes);

    const auto header = detail::read_header(input_bits);
    expect(header.has_value())
        << "got error: " << static_cast<int>(header.error());
    expect(header->type == detail::BlockType::FixedHuffman);

    const std::vector<std::byte> expected_bytes =
        read_runfile(*argv, "starflate/src/test/starfleet.html");
    std::vector<std::byte> dst(expected_bytes.size());
    const auto status = decompress(input_bytes, dst);
    expect(status == DecompressStatus::Success)
        << "got error code: " << static_cast<int>(status);
    expect(std::ranges::equal(dst, expected_bytes))
        << "decompressed does not match expected";
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

  test("copy_from_before") = [] {
    auto src_and_dst = huffman::byte_array(1, 2, 0, 0, 0, 0);
    const auto dst_span = std::span<std::byte>{src_and_dst}.subspan(2);
    detail::copy_from_before(dst_span.begin(), 2, 3);
    expect(eq(src_and_dst, huffman::byte_array(1, 2, 1, 2, 1, 0)));
  };
};
