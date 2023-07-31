#include <benchmark/benchmark.h>
#include <glm/glm.hpp>

static void BM_VectorSubtractNew(benchmark::State& state){
    for(auto _ : state){
        auto res = glm::vec2(1) - glm::vec2(0.5);
        benchmark::DoNotOptimize(res);
    }
}

static void BM_VectorSubtractSaved(benchmark::State& state){
    glm::vec2 res{1};
    for(auto _ : state){
        res -= glm::vec2(0.5);
    }
}

// Register the function as a benchmark
BENCHMARK(BM_VectorSubtractNew);
BENCHMARK(BM_VectorSubtractSaved);
// Run the benchmark
BENCHMARK_MAIN();