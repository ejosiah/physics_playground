#include "matrix.h"
#include "test_matrix_generator.h"

#include "linear_systems.h"
#include "preconditioner.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <random>
#include <numeric>
#include <format>


struct ichoPreconditioner {

    template<typename Matrix>
    Matrix operator()(const Matrix& matrix) const {
        auto M =  precondition(matrix);
        return lowerTriangularInvert(M);
    }
};

class LinearSystemsFixture : public ::testing::Test {
protected:
    void SetUp() override {
        Test::SetUp();
    }

    void TearDown() override {
        Test::TearDown();
    }
};

using namespace blas;

TEST_F(LinearSystemsFixture, jacobiSolver) {
    Vector x(3);
    Vector b{-1, 2, 3};
    Matrix A{
        {5, -2, 3},
        {-3, 9, 1},
        {2, -1, -7}
    };

    auto iterations = lns::jacobi(A, x, b, 0.0003);
    std::printf("iterations %zu\n", iterations);

    ASSERT_NEAR(0.186, x[0], 0.01);
    ASSERT_NEAR(0.331, x[1], 0.01);
    ASSERT_NEAR(-0.423, x[2], 0.01);
}

TEST_F(LinearSystemsFixture, gaussSeidelSolver) {
    Vector x(3);
    Vector b{-1, 2, 3};
    Matrix A{
        {5, -2, 3},
        {-3, 9, 1},
        {2, -1, -7}
    };

    auto iterations = lns::gauss_seidel(A, x, b, 0.0003);
    std::printf("iterations %zu\n", iterations);

    ASSERT_NEAR(0.186, x[0], 0.01);
    ASSERT_NEAR(0.331, x[1], 0.01);
    ASSERT_NEAR(-0.423, x[2], 0.01);
}

TEST_F(LinearSystemsFixture, GradientDescentSolver) {
    Vector x(2);
    Vector b{2, -8};
    Matrix A{
        {3, 2},
        {2, 6}
    };

    auto iterations = lns::gradient_descent(A, x, b, 0.0003);
    std::printf("iterations %zu\n", iterations);

    ASSERT_NEAR(2, x[0], 0.01);
    ASSERT_NEAR(-2, x[1], 0.01);
}

TEST_F(LinearSystemsFixture, ConjugateGradientSolver) {
    Vector x(2);
    Vector b{2, -8};
    Matrix A{
            {3, 2},
            {2, 6}
    };

    auto iterations = lns::conjugate_gradient(A, x, b, lns::IdentityPreconditioner{}, 0.0003);
    std::printf("iterations %zu\n", iterations);

    ASSERT_NEAR(2, x[0], 0.01);
    ASSERT_NEAR(-2, x[1], 0.01);
}

TEST_F(LinearSystemsFixture, playground) {
    Matrix A{
            {4, 1, -1, 0, 0, 0, 0, 0},
            {1, 6, -2, 1, -1, 0, 0, 0},
            {0, 1, 5, 0, -1, 1, 0, 0},
            {0, 2, 0, 5, -1, 0, -1, -1},
            {0, 0, -1, -1, 6, -1, 0, -1},
            {0, 0, -1, 0, -1, 5, 0, 0},
            {0, 0, 0, -1, 0, 0, 4, -1},
            {0, 0, 0, -1, -1, 0, -1, 5}

    };
    Vector b{3, -6, -5, 0, 12, -12, -2, 2};
    Vector x(8);

    auto threshold = 1e-10;
    auto iterations = lns::gauss_seidel(A, x, b, threshold);

    std::printf("gauss_seidel, iterations(%zu):\n\t", iterations);
    for(auto v : x){
        std::printf("%f ", v);
    }

    x.clear();
    iterations = lns::jacobi(A, x, b, threshold);
    std::printf("\njacobi, iterations(%zu):\n\t", iterations);
    for(auto v : x){
        std::printf("%f ", v);
    }

    x.clear();
    iterations = lns::gradient_descent(A, x, b, threshold);
    std::printf("\ngradient_descent, iterations(%zu):\n\t", iterations);
    for(auto v : x){
        std::printf("%f ", v);
    }

    auto pc = ichoPreconditioner{};
    iterations = 8;
    x.clear();
    iterations = lns::conjugate_gradient(A, x, b, pc, threshold, iterations);
    std::printf("\nconjugate_gradient, iterations(%zu):\n\t", iterations);
    for(auto v : x){
        std::printf("%f ", v);
    }
}

TEST_F(LinearSystemsFixture, loadMatrix) {
//    SparseMatrix matrix = load<float>(R"(C:\Users\Josiah Ebhomenye\CLionProjects\physics_playground\common\data\1138_bus.mtx)");
    SparseMatrix matrix = generatePoissonEquationMatrix(32);
    Vector expected(matrix.rows());

//    std::cout << matrix << "\n";

    std::default_random_engine engine{ (1 << 20) };
    std::uniform_int_distribution<int> dist{-100, 100};

    std::generate(expected.begin(), expected.end(), [&]{ return static_cast<float>(dist(engine)); });

    Vector b = matrix * expected;
    Vector x(matrix.rows());
    auto pc = lns::IdentityPreconditioner{};
    auto A = static_cast<MatrixT<float>>(matrix);
    auto iterations = 0;
   // lns::jacobi(matrix, x, b, size_t{10});
    //iterations = lns::conjugate_gradient(matrix, x, b, pc, 1e-4, iterations);
    iterations = lns::gradient_descent(matrix, x, b, 1e-4, 10u);
    std::cout << std::format("found solution after {} iterations\n", iterations);

    for(int i = 0; i < 10; i++){
        std::cout << std::format("{} <=> {}\n", expected[i], x[i]);
    }

    for(auto i = 0; i < matrix.rows(); i++){
        ASSERT_NEAR(expected[i], x[i], 0.5);
    }
}

TEST_F(LinearSystemsFixture, sparseMatrix) {
    SparseMatrix A{
            { {0, 4}, {1, 1}, {2, -1} },
            { {0, 1}, {1, 6}, {2, -2}, {3, 1}, {4, -1} },
            { {1, 1}, {2, 5}, {4, -1}, {5, 1} },
            { {1, 2}, {3, 5}, {4, -1}, {6, -1}, {7, -1} },
            { {2, -1}, {3, -1}, {4, 6}, {5, -1}, {7, -1} },
            { {2, -1}, {4, -1}, {5, 5} },
            { {3, -1}, {6, 4}, {7, -1} },
            { {3, -1}, {4, -1}, {6, -1}, {7, 5} }
    };

    std::vector<int> counts{3, 5, 4, 5, 5, 3, 3, 4};

    std::exclusive_scan(counts.begin(), counts.end(), counts.begin(), 0);
    for(auto c : counts){
        printf("%d ", c);
    }

    Vector b{3, -6, -5, 0, 12, -12, -2, 2};
    Vector x1(8);


    Matrix AA{
            {4, 1, -1, 0, 0, 0, 0, 0},
            {1, 6, -2, 1, -1, 0, 0, 0},
            {0, 1, 5, 0, -1, 1, 0, 0},
            {0, 2, 0, 5, -1, 0, -1, -1},
            {0, 0, -1, -1, 6, -1, 0, -1},
            {0, 0, -1, 0, -1, 5, 0, 0},
            {0, 0, 0, -1, 0, 0, 4, -1},
            {0, 0, 0, -1, -1, 0, -1, 5}

    };

    Vector x2(8);


    auto iterationsB = lns::gauss_seidel(AA, x2, b, 0.0001);
    auto iterationsA = lns::gauss_seidel(A, x1, b, 0.0001);


    ASSERT_EQ(iterationsA, iterationsB);

    ASSERT_FLOAT_EQ(x1[0], x2[0]);
    ASSERT_FLOAT_EQ(x1[1], x2[1]);
    ASSERT_FLOAT_EQ(x1[2], x2[2]);
    ASSERT_FLOAT_EQ(x1[3], x2[3]);
    ASSERT_FLOAT_EQ(x1[4], x2[4]);
    ASSERT_FLOAT_EQ(x1[5], x2[5]);
    ASSERT_FLOAT_EQ(x1[6], x2[6]);
    ASSERT_FLOAT_EQ(x1[7], x2[7]);

    x1.clear();
    x2.clear();

    iterationsA = lns::jacobi(A, x1, b, 0.0001);
    iterationsB = lns::jacobi(AA, x2, b, 0.0001);

    ASSERT_EQ(iterationsA, iterationsB);

    ASSERT_FLOAT_EQ(x1[0], x2[0]);
    ASSERT_FLOAT_EQ(x1[1], x2[1]);
    ASSERT_FLOAT_EQ(x1[2], x2[2]);
    ASSERT_FLOAT_EQ(x1[3], x2[3]);
    ASSERT_FLOAT_EQ(x1[4], x2[4]);
    ASSERT_FLOAT_EQ(x1[5], x2[5]);
    ASSERT_FLOAT_EQ(x1[6], x2[6]);
    ASSERT_FLOAT_EQ(x1[7], x2[7]);

    x1.clear();
    x2.clear();

    iterationsA = lns::gradient_descent(A, x1, b, 0.0001);
    iterationsB = lns::gradient_descent(AA, x2, b, 0.0001);

    ASSERT_EQ(iterationsA, iterationsB);

    ASSERT_FLOAT_EQ(x1[0], x2[0]);
    ASSERT_FLOAT_EQ(x1[1], x2[1]);
    ASSERT_FLOAT_EQ(x1[2], x2[2]);
    ASSERT_FLOAT_EQ(x1[3], x2[3]);
    ASSERT_FLOAT_EQ(x1[4], x2[4]);
    ASSERT_FLOAT_EQ(x1[5], x2[5]);
    ASSERT_FLOAT_EQ(x1[6], x2[6]);
    ASSERT_FLOAT_EQ(x1[7], x2[7]);

    x1.clear();
    x2.clear();

//    iterationsA = lns::conjugate_gradient(A, x1, b, 0.0001);
//    iterationsB = lns::conjugate_gradient(AA, x2, b, 0.0001);
//
//    ASSERT_EQ(iterationsA, iterationsB);
//
//    ASSERT_FLOAT_EQ(x1[0], x2[0]);
//    ASSERT_FLOAT_EQ(x1[1], x2[1]);
//    ASSERT_FLOAT_EQ(x1[2], x2[2]);
//    ASSERT_FLOAT_EQ(x1[3], x2[3]);
//    ASSERT_FLOAT_EQ(x1[4], x2[4]);
//    ASSERT_FLOAT_EQ(x1[5], x2[5]);
//    ASSERT_FLOAT_EQ(x1[6], x2[6]);
//    ASSERT_FLOAT_EQ(x1[7], x2[7]);
}
