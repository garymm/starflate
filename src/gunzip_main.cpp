#include "gunzip.hpp"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ostream>

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)

auto usage(const char* program) -> int
{
  std::cerr << "Usage: " << program << "[OPTIONS] <filename>" << std::endl;
  std::cerr << "OPTIONS:" << std::endl;
  std::cerr
      << "\n\t-c --stdout --to-stdout\n\t\tWrite to standard output"
      << std::endl;
  return EXIT_FAILURE;
}

auto decompressed_path(const std::filesystem::path& in_path)
    -> std::filesystem::path
{
  std::filesystem::path res{in_path};
  if (in_path.extension() == ".gz") {
    res.replace_extension();
  } else {
    res += ".decompressed";
  }
  return res;
}

auto main(const int argc, const char* argv[]) -> int
{
  if (argc < 2 || argc > 3) {
    return usage(argv[0]);
  }

  std::filesystem::path in_path;
  bool to_stdout{};
  std::ofstream out_file;
  if (argc == 3) {
    const auto option = argv[1];
    // NOLINTBEGIN(readability-magic-numbers)
    if (strncmp(option, "-c", 2) != 0 && strncmp(option, "--stdout", 8) != 0 &&
        strncmp(option, "--to-stdout", 11) != 0) {
      // NOLINTEND(readability-magic-numbers)
      return usage(argv[0]);
    }
    in_path = argv[2];
    to_stdout = true;
  } else {
    in_path = argv[1];
    std::filesystem::path out_path{decompressed_path(in_path)};
    out_file.open(out_path, std::ios::binary);
    if (!out_file.is_open()) {
      std::cerr << "Failed to open " << out_path << " for writing" << std::endl;
      return EXIT_FAILURE;
    }
  }
  // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  auto& out = to_stdout ? std::cout : out_file;

  std::ifstream in{in_path, std::ios::binary};
  if (!in.is_open()) {
    std::cerr << "Failed to open " << in_path << " for reading" << std::endl;
    return EXIT_FAILURE;
  }

  const auto err = starflate::gunzip(in, out);
  if (err != starflate::GunzipError::NoError) {
    std::cerr << "Error: " << static_cast<int>(err) << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
