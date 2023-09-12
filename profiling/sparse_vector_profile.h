#include "big_vector.h"
#include <benchmark/benchmark.h>
#include <random>
#include <algorithm>
#include <map>
#include <iostream>

class SparseVectorFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State &state) override {
        Fixture::SetUp(state);
    }

    void TearDown(const benchmark::State &state) override {
        Fixture::TearDown(state);
    }

};

using namespace blas;

BENCHMARK_DEFINE_F(SparseVectorFixture, addition)(benchmark::State& state) {
    SparseVector a{{0, 1}, {2, 3}, {4, 5}, {6, 8}, {9, 20}};
    Vector b(state.range(0));

    std::default_random_engine engine{ (1 << 20) };
    std::uniform_real_distribution<float> dist{-100, 100};

    std::generate(b.begin(), b.end(), [&]{ return dist(engine); });


    for(auto _ : state){
        auto c = a + b;
        BENCHMARK_DONT_OPTIMIZE(c);
    }
}

BENCHMARK_DEFINE_F(SparseVectorFixture, subtraction)(benchmark::State& state) {
    SparseVector a{{0, 1}, {2, 3}, {4, 5}, {6, 8}, {9, 20}};
    Vector b(state.range(0));

    std::default_random_engine engine{ (1 << 20) };
    std::uniform_real_distribution<float> dist{-100, 100};

    std::generate(b.begin(), b.end(), [&]{ return dist(engine); });


    for(auto _ : state){
        auto c = a - b;
        BENCHMARK_DONT_OPTIMIZE(c);
    }
}

BENCHMARK_DEFINE_F(SparseVectorFixture, multiplication)(benchmark::State& state) {
    SparseVector a{{0, 1}, {2, 3}, {4, 5}, {6, 8}, {9, 20}};
    Vector b(state.range(0));

    std::default_random_engine engine{ (1 << 20) };
    std::uniform_real_distribution<float> dist{-100, 100};

    std::generate(b.begin(), b.end(), [&]{ return dist(engine); });


    for(auto _ : state){
        auto c = a * b;
        BENCHMARK_DONT_OPTIMIZE(c);
    }
}

BENCHMARK_DEFINE_F(SparseVectorFixture, dotProduct)(benchmark::State& state) {
    SparseVector a{{0, 1}, {2, 3}, {4, 5}, {6, 8}, {9, 20}};
    Vector b(state.range(0));

    std::default_random_engine engine{ (1 << 20) };
    std::uniform_real_distribution<float> dist{-100, 100};

    std::generate(b.begin(), b.end(), [&]{ return dist(engine); });

    for(auto _ : state){
        auto c = a.dot(b);
        BENCHMARK_DONT_OPTIMIZE(c);
    }
}


BENCHMARK_REGISTER_F(SparseVectorFixture, addition)->RangeMultiplier(2)->Range(32, 32 << 5);
BENCHMARK_REGISTER_F(SparseVectorFixture, subtraction)->RangeMultiplier(2)->Range(32, 32 << 5);
BENCHMARK_REGISTER_F(SparseVectorFixture, multiplication)->RangeMultiplier(2)->Range(32, 32 << 5);
BENCHMARK_REGISTER_F(SparseVectorFixture, dotProduct)->RangeMultiplier(2)->Range(32, 32 << 5);