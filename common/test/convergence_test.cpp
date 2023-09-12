#include <gtest/gtest.h>
#include "matrix.h"
#include "linear_systems.h"
#include "test_matrix_generator.h"
#include <random>
#include <algorithm>
#include <iostream>
#include <format>
#include <string>
#include <vector>
#include "profile.h"
#include <future>
#include <daw/json/daw_to_json.h>
#include <daw/json/daw_json_link.h>

constexpr int ITERATIONS = 100;
constexpr double TOLERANCE = 1e-10;
const lns::IdentityPreconditioner pc{};

struct Result {
    std::string solver;
    std::vector<float> error{};
    std::vector<int> iterationsUsed{};
    std::vector<float> time{};
};




namespace daw::json {
    template<>
    struct json_data_contract<Result> {
        using type = json_member_list<
                json_string<"solver", std::string>,
                json_array<"error", float>,
                json_array<"iterationsUsed", int>,
                json_array<"time", float>
        >;

        static inline auto
        to_json_data( Result const &result ) {
            return std::forward_as_tuple( result.solver, result.error, result.iterationsUsed, result.time );
        }
    };
} // namespace daw::json



struct JacobiSolver {
    std::string name{"Jacobi"};

    int operator()(blas::SparseMatrix& A, blas::Vector& x, blas::Vector& b, size_t iterations) {
        lns::jacobi(A, x, b, iterations);
        return iterations;
    }
};

struct GaussSeidelSolver {
    std::string name{"Gauss Seidel"};

    int operator()(blas::SparseMatrix& A, blas::Vector& x, blas::Vector& b, size_t iterations) {
        lns::gauss_seidel(A, x, b, iterations);
        return iterations;
    }
};


struct GradientDescentSolver {
    std::string name{"Gradient Descent"};

    int operator()(blas::SparseMatrix& A, blas::Vector& x, blas::Vector& b, size_t iterations) {
        return lns::gradient_descent(A, x, b, TOLERANCE, iterations);
    }
};

struct ConjugateGradientSolver {
    std::string name{"Conjugate Gradient"};

    int operator()(blas::SparseMatrix& A, blas::Vector& x, blas::Vector& b, size_t iterations) {
        return lns::conjugate_gradient(A, x, b, pc, TOLERANCE, iterations);
    }
};

template<typename Solver>
Result run(blas::SparseMatrix A, const blas::Vector& x, blas::Vector b) {
    Solver solver{};
    Result result{solver.name};
    result.error = std::vector<float>(ITERATIONS);
    result.iterationsUsed = std::vector<int>(ITERATIONS);
    result.time = std::vector<float>(ITERATIONS);

    blas::Vector x0(x.size());
    for(size_t i = 0; i < ITERATIONS; i++){
        x0.clear();
        auto& iterations = result.iterationsUsed[i];
        auto& error = result.error[i];

        auto time = profile<chrono::nanoseconds>([&]{
            iterations = solver(A, x0, b, i + 1);
            error = (x - x0).length();
        });
        result.time[i] = static_cast<float>(time.count());

    }
    return result;
}


TEST(LinearSystemsSolvers, convergence) {
    std::map<size_t, std::vector<std::future<Result>>> futures{};
    for(int i = 0; i < 6; i++){
        const auto N = (32 << i);
        auto A = generatePoissonEquationMatrix(N);
        std::uniform_real_distribution<float> dist{-10, 10};
        std::default_random_engine engine{1 << 20};
        blas::Vector solution(A.rows());
        blas::Vector x(A.rows());

        std::generate(solution.begin(), solution.end(), [&]{ return dist(engine); });
        auto b = A * solution;

        std::vector<std::future<Result>> rFutures(4);

        std::cout << std::format("running convergence tests for {} entries\n", N);

        rFutures[0] = std::async(run<JacobiSolver>, A, solution, b);
        rFutures[1] = std::async(run<GaussSeidelSolver>, A, solution, b);
        rFutures[2] = std::async(run<GradientDescentSolver>, A, solution, b);
        rFutures[3] = std::async(run<ConjugateGradientSolver>, A, solution, b);
        futures[N] = std::move(rFutures);
    }

   std::map<size_t, std::vector<Result>> results{};

    for(auto& [size, sFutures] : futures){
        std::vector<Result> rResults{};
        for(auto& rFutures : sFutures){
            rResults.push_back(rFutures.get());
        }
        results[size] = rResults;
    }

    auto json = daw::json::to_json(results);
    std::cout << json << "\n";
}