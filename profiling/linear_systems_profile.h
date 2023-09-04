#pragma once

#include "matrix.h"
#include "linear_systems.h"
#include "test_matrix_generator.h"
#include <benchmark/benchmark.h>
#include <random>
#include <algorithm>
#include <map>

class LinearSystemsFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State &state) override {
        gs_iterations[8] = 10;
        gs_iterations[16] = 10;
        gs_iterations[32] = 18;
        gs_iterations[64] = 18;
        gs_iterations[128] = 19;
        gs_iterations[256] = 19;
        gs_iterations[512] = 19;
        gs_iterations[1024] = 19;
        gs_iterations[2048] = 20;
        gs_iterations[4096] = 20;
        gs_iterations[8192] = 20;

        jc_iterations[8] = 30;
        jc_iterations[16] = 31;
        jc_iterations[32] = 34;
        jc_iterations[64] = 34;
        jc_iterations[128] = 34;
        jc_iterations[256] = 34;
        jc_iterations[512] = 34;
        jc_iterations[1024] = 34;
        jc_iterations[2048] = 34;
        jc_iterations[4096] = 35;
        jc_iterations[8192] = 35;
    }

    void TearDown(const benchmark::State &state) override {
        Fixture::TearDown(state);
    }


    static constexpr double Tolerance = 1e-4;
    lns::IdentityPreconditioner pc{};
    std::map<size_t, size_t> gs_iterations;
    std::map<size_t, size_t> jc_iterations;
};




BENCHMARK_DEFINE_F(LinearSystemsFixture, jacobiSolverDenseMatrix)(benchmark::State& state) {
    // generate test data
    blas::Matrix A = generateMatrix(state.range(0));
    std::default_random_engine engine{ (1 << 20) };
    std::uniform_int_distribution<int> dist{-100, 100};
    blas::Vector x(state.range(0));
    std::generate(x.begin(), x.end(), [&]{ return static_cast<float>(dist(engine)); });
    auto b = A * x;
    x.clear();

    size_t iterations{jc_iterations[state.range(0)]};
    for(auto _ : state) {
        state.PauseTiming();
        x.clear();
        state.ResumeTiming();
        lns::jacobi(A, x, b, iterations);
    }
    state.counters["lns_iterations"] = static_cast<double>(iterations);
}

BENCHMARK_DEFINE_F(LinearSystemsFixture, jacobiSparseMatrix)(benchmark::State& state) {
    // generate test data
    blas::SparseMatrix A = generateMatrix(state.range(0));
    std::default_random_engine engine{ (1 << 20) };
    std::uniform_int_distribution<int> dist{-100, 100};
    blas::Vector x(state.range(0));
    std::generate(x.begin(), x.end(), [&]{ return static_cast<float>(dist(engine)); });
    auto b = A * x;

    size_t iterations{jc_iterations[state.range(0)]};
    for(auto _ : state) {
        state.PauseTiming();
        x.clear();
        state.ResumeTiming();
        lns::jacobi(A, x, b, iterations);
    }
    state.counters["lns_iterations"] = static_cast<double>(iterations);
}

BENCHMARK_DEFINE_F(LinearSystemsFixture, gaussSeidelDenseMatrix)(benchmark::State& state) {
    // generate test data
    blas::Matrix A = generateMatrix(state.range(0));
    std::default_random_engine engine{ (1 << 20) };
    std::uniform_int_distribution<int> dist{-100, 100};
    blas::Vector x(state.range(0));
    std::generate(x.begin(), x.end(), [&]{ return static_cast<float>(dist(engine)); });
    auto b = A * x;
    x.clear();

    size_t iterations{gs_iterations[state.range(0)]};
    for(auto _ : state) {
        state.PauseTiming();
        x.clear();
        state.ResumeTiming();
        lns::gauss_seidel(A, x, b, iterations);
    }
    state.counters["lns_iterations"] = static_cast<double>(iterations);
}

BENCHMARK_DEFINE_F(LinearSystemsFixture, gaussSeidelSparseMatrix)(benchmark::State& state) {
    // generate test data
    blas::SparseMatrix A = generateMatrix(state.range(0));
    std::default_random_engine engine{ (1 << 20) };
    std::uniform_int_distribution<int> dist{-100, 100};
    blas::Vector x(state.range(0));
    std::generate(x.begin(), x.end(), [&]{ return static_cast<float>(dist(engine)); });
    auto b = A * x;

    size_t iterations{gs_iterations[state.range(0)]};
    for(auto _ : state) {
        state.PauseTiming();
        x.clear();
        state.ResumeTiming();
        lns::gauss_seidel(A, x, b, iterations);
    }
    state.counters["lns_iterations"] = static_cast<double>(iterations);
}

BENCHMARK_DEFINE_F(LinearSystemsFixture, gradientDesentDenseMatrix)(benchmark::State& state) {
    // generate test data
    blas::Matrix A = generateMatrix(state.range(0));
    std::default_random_engine engine{ (1 << 20) };
    std::uniform_int_distribution<int> dist{-100, 100};
    blas::Vector x(state.range(0));
    std::generate(x.begin(), x.end(), [&]{ return static_cast<float>(dist(engine)); });
    auto b = A * x;
    x.clear();

    size_t iterations{};
    for(auto _ : state) {
        state.PauseTiming();
        x.clear();
        state.ResumeTiming();
        iterations += lns::gradient_descent(A, x, b, Tolerance, state.range(0) * 5);
    }
    state.counters["lns_iterations"] = static_cast<double>(iterations)/state.iterations();
}

BENCHMARK_DEFINE_F(LinearSystemsFixture, gradientDesentSparseMatrix)(benchmark::State& state) {
    // generate test data
    blas::SparseMatrix A = generateMatrix(state.range(0));
    std::default_random_engine engine{ (1 << 20) };
    std::uniform_int_distribution<int> dist{-100, 100};
    blas::Vector x(state.range(0));
    std::generate(x.begin(), x.end(), [&]{ return static_cast<float>(dist(engine)); });
    auto b = A * x;
    x.clear();

    size_t iterations{};
    for(auto _ : state) {
        state.PauseTiming();
        x.clear();
        state.ResumeTiming();
        iterations += lns::gradient_descent(A, x, b, Tolerance, state.range(0) * 5);
    }
    state.counters["lns_iterations"] = static_cast<double>(iterations)/state.iterations();
}

BENCHMARK_DEFINE_F(LinearSystemsFixture, conjugateGradientDenseMatrix)(benchmark::State& state) {
    // generate test data
    blas::Matrix A = generateMatrix(state.range(0));
    std::default_random_engine engine{ (1 << 20) };
    std::uniform_int_distribution<int> dist{-100, 100};
    blas::Vector x(state.range(0));
    std::generate(x.begin(), x.end(), [&]{ return static_cast<float>(dist(engine)); });
    auto b = A * x;
    x.clear();

    size_t iterations{};
    for(auto _ : state) {
        state.PauseTiming();
        x.clear();
        state.ResumeTiming();
        iterations += lns::conjugate_gradient(A, x, b, pc, Tolerance, x.size());
    }
    state.counters["lns_iterations"] = static_cast<double>(iterations)/state.iterations();
}

BENCHMARK_DEFINE_F(LinearSystemsFixture, conjugateGradientSparseMatrix)(benchmark::State& state) {
    // generate test data
    blas::Matrix A = generateMatrix(state.range(0));
    std::default_random_engine engine{ (1 << 20) };
    std::uniform_int_distribution<int> dist{-100, 100};
    blas::Vector x(state.range(0));
    std::generate(x.begin(), x.end(), [&]{ return static_cast<float>(dist(engine)); });
    auto b = A * x;
    x.clear();

    size_t iterations{};
    for(auto _ : state) {
        state.PauseTiming();
        x.clear();
        state.ResumeTiming();
        iterations += lns::conjugate_gradient(A, x, b, pc, Tolerance, x.size());
    }
    state.counters["lns_iterations"] = static_cast<double>(iterations)/state.iterations();
}


BENCHMARK_REGISTER_F(LinearSystemsFixture, jacobiSolverDenseMatrix)->RangeMultiplier(2)->Range(8, 8<<10);
BENCHMARK_REGISTER_F(LinearSystemsFixture, jacobiSparseMatrix)->RangeMultiplier(2)->Range(8, 8<<10);
BENCHMARK_REGISTER_F(LinearSystemsFixture, gaussSeidelDenseMatrix)->RangeMultiplier(2)->Range(8, 8<<10);
BENCHMARK_REGISTER_F(LinearSystemsFixture, gaussSeidelSparseMatrix)->RangeMultiplier(2)->Range(8, 8<<10);
BENCHMARK_REGISTER_F(LinearSystemsFixture, gradientDesentDenseMatrix)->RangeMultiplier(2)->Range(8, 8<<10);
BENCHMARK_REGISTER_F(LinearSystemsFixture, gradientDesentSparseMatrix)->RangeMultiplier(2)->Range(8, 8<<10);
BENCHMARK_REGISTER_F(LinearSystemsFixture, conjugateGradientDenseMatrix)->RangeMultiplier(2)->Range(8, 8<<10);
BENCHMARK_REGISTER_F(LinearSystemsFixture, conjugateGradientSparseMatrix)->RangeMultiplier(2)->Range(8, 8<<10);