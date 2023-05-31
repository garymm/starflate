#include "src/huffman.hpp"

#include <benchmark/benchmark.h>

namespace {

void BM_CodeTable(benchmark::State& state)
{
  const auto frequencies = std::vector<std::pair<char, std::size_t>>{
      {'e', 100}, {'n', 20}, {'x', 1}, {'i', 40}, {'q', 3}};
  constexpr auto eot = char{4};

  for (auto _ : state) {
    gpu_deflate::code_table ct{frequencies, eot};
    benchmark::DoNotOptimize(ct);
  }
}

BENCHMARK(BM_CodeTable);

}  // namespace

BENCHMARK_MAIN();
