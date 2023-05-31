#include <benchmark/benchmark.h>

#include "src/huffman.hpp"

static void BM_CodeTable(benchmark::State& state) {
  // Perform setup here
  const auto frequencies = std::vector<std::pair<char, std::size_t>>{
        {'e', 100}, {'n', 20}, {'x', 1}, {'i', 40}, {'q', 3}};
  constexpr auto eot = char{4};

  for (auto _ : state) {
    gpu_deflate::code_table ct{frequencies, eot};
    benchmark::DoNotOptimize(ct);
  }
}
// Register the function as a benchmark
BENCHMARK(BM_CodeTable);
// Run the benchmark
BENCHMARK_MAIN();
