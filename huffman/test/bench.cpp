#include "huffman/huffman.hpp"
#include "version/version.hpp"

#include <array>
#include <benchmark/benchmark.h>

// ignore checks to Google Benchmark headers,
// NOLINTBEGIN(clang-analyzer-deadcode.DeadStores,cppcoreguidelines-avoid-non-const-global-variables,cppcoreguidelines-owning-memory,modernize-use-trailing-return-type)

namespace {

void BM_CodeTable(benchmark::State& state)
{
  const auto frequencies = std::vector<std::pair<char, std::size_t>>{
      {{'e', 100}, {'n', 20}, {'x', 1}, {'i', 40}, {'q', 3}}};
  constexpr auto eot = char{4};

  state.SetLabel(gpu_deflate::Version::full_version_string);
  for (auto _ : state) {
    // NOLINTNEXTLINE(readability-magic-numbers)
    auto ct = gpu_deflate::huffman::table<char, 6>{frequencies, eot};
    benchmark::DoNotOptimize(ct);
  }
}

BENCHMARK(BM_CodeTable);

}  // namespace

BENCHMARK_MAIN();

// NOLINTEND(clang-analyzer-deadcode.DeadStores,cppcoreguidelines-avoid-non-const-global-variables,cppcoreguidelines-owning-memory,modernize-use-trailing-return-type)
