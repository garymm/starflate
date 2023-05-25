#include "src/huffman.hpp"
#include "src/version/version.hpp"

#include <array>
#include <benchmark/benchmark.h>

namespace {

void BM_CodeTable(benchmark::State& state)
{
  const auto frequencies = std::vector<std::pair<char, std::size_t>>{
      {{'e', 100}, {'n', 20}, {'x', 1}, {'i', 40}, {'q', 3}}};
  constexpr auto eot = char{4};

  state.SetLabel(gpu_deflate::Version::full_version_string);
  for (auto _ : state) {
    auto ct = gpu_deflate::code_table<char, 6>{frequencies, eot};
    benchmark::DoNotOptimize(ct);
  }
}

BENCHMARK(BM_CodeTable);

}  // namespace

BENCHMARK_MAIN();
