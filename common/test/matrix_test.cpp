#include "matrix.h"
#include "linear_systems.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

class MatrixFixture : public ::testing::Test {
protected:
    void SetUp() override {
        Test::SetUp();
    }

    void TearDown() override {
        Test::TearDown();
    }
};

using namespace blas;

TEST_F(MatrixFixture, transpose) {
    Matrix matrix{
            {1, 2, 3, 4, 5, 6, 7, 8},
            {1, 2, 3, 4, 5, 6, 7, 8},
            {1, 2, 3, 4, 5, 6, 7, 8},
            {1, 2, 3, 4, 5, 6, 7, 8},
            {1, 2, 3, 4, 5, 6, 7, 8},
            {1, 2, 3, 4, 5, 6, 7, 8},
            {1, 2, 3, 4, 5, 6, 7, 8},
            {1, 2, 3, 4, 5, 6, 7, 8},
    };

    auto result = transpose(matrix);

    for(int i = 0; i < 8; ++i){
        for(auto j = 0; j < 8; ++j){
            ASSERT_EQ(float(i+1), result[i][j]);
        }
    }
}

TEST_F(MatrixFixture, identityMatrix) {
    auto matrix = Matrix(8, 8).identity();

    for(int i = 0; i < 8; ++i){
        for(int j = 0; j < 8; ++j){
            if(i == j){
                ASSERT_EQ(matrix[i][j], 1);
            }else {
                ASSERT_EQ(matrix[i][j], 0);
            }
        }
    }
}

TEST_F(MatrixFixture, matrixAddition) {
    Matrix A{
            { 2,  3,  1,  4,  5,  6,  7,  8 },
            {-1,  0,  2,  1,  3,  2,  0,  1 },
            { 4,  2,  5, -1,  2,  1,  3,  2 },
            { 0,  1,  2,  3,  4,  5,  6,  7 },
            { 3,  1,  0,  2,  1,  3,  2,  0 },
            { 1, -2, -3,  4,  0, -1, -2, -3 },
            { 2,  1,  2,  1,  2,  1,  2,  1 },
            {-1,  0,  1,  0,  1,  0,  1,  0 }
    };

    Matrix B{
            { 1,  2,  3,  4, -1, -2, -3, -4},
            { 2,  3,  0,  1,  0,  1,  0,  1},
            { 0,  1,  2,  3,  4,  5,  6,  7},
            {1, -2,  1,  2, -1, -2,  1,  2},
            { 3,  4,  5,  6,  2,  3,  4,  5},
            { 0,  1,  0,  1,  0,  1,  0,  1},
            { 1,  2,  3,  4, -1, -2, -3, -4},
            { 2,  3,  0,  1,  0,  1,  0,  1},
    };

    Matrix expected{
            {3, 5, 4, 8, 4, 4, 4, 4},
            {1, 3, 2, 2, 3, 3, 0, 2},
            {4, 3, 7, 2, 6, 6, 9, 9},
            {1, -1, 3, 5, 3, 3, 7, 9},
            {6, 5, 5, 8, 3, 6, 6, 5},
            {1, -1, -3, 5, 0, 0, -2, -2},
            {3, 3, 5, 5, 1, -1, -1, -3},
            {1, 3, 1, 1, 1, 1, 1, 1},
    };

    auto C = A + B;

    for(auto i = 0; i < 8; ++i){
        for(auto j = 0; j < 8; ++j){
            ASSERT_FLOAT_EQ(expected[i][j], C[i][j]);
        }
    }

}

TEST_F(MatrixFixture, matrixSubtraction) {
    Matrix A{
            { 2,  3,  1,  4,  5,  6,  7,  8 },
            {-1,  0,  2,  1,  3,  2,  0,  1 },
            { 4,  2,  5, -1,  2,  1,  3,  2 },
            { 0,  1,  2,  3,  4,  5,  6,  7 },
            { 3,  1,  0,  2,  1,  3,  2,  0 },
            { 1, -2, -3,  4,  0, -1, -2, -3 },
            { 2,  1,  2,  1,  2,  1,  2,  1 },
            {-1,  0,  1,  0,  1,  0,  1,  0 }
    };

    Matrix B{
            { 1,  2,  3,  4, -1, -2, -3, -4},
            { 2,  3,  0,  1,  0,  1,  0,  1},
            { 0,  1,  2,  3,  4,  5,  6,  7},
            {1, -2,  1,  2, -1, -2,  1,  2},
            { 3,  4,  5,  6,  2,  3,  4,  5},
            { 0,  1,  0,  1,  0,  1,  0,  1},
            { 1,  2,  3,  4, -1, -2, -3, -4},
            { 2,  3,  0,  1,  0,  1,  0,  1},
    };

    Matrix expected{
            {1, 1, -2, 0, 6, 8, 10, 12},
            {-3, -3, 2, 0, 3, 1, 0, 0},
            {4, 1, 3, -4, -2, -4, -3, -5},
            {-1, 3, 1, 1, 5, 7, 5, 5},
            {0, -3, -5, -4, -1, 0, -2, -5},
            {1, -3, -3, 3, 0, -2, -2, -4},
            {1, -1, -1, -3, 3, 3, 5, 5},
            {-3, -3, 1, -1, 1, -1, 1, -1},
    };

    auto C = A - B;

    for(auto i = 0; i < 8; ++i){
        for(auto j = 0; j < 8; ++j){
            ASSERT_FLOAT_EQ(expected[i][j], C[i][j]);
        }
    }
}

TEST_F(MatrixFixture, matrixMultiplication) {
    Matrix A{
            { 2,  3,  1,  4,  5,  6,  7,  8 },
            {-1,  0,  2,  1,  3,  2,  0,  1 },
            { 4,  2,  5, -1,  2,  1,  3,  2 },
            { 0,  1,  2,  3,  4,  5,  6,  7 },
            { 3,  1,  0,  2,  1,  3,  2,  0 },
            { 1, -2, -3,  4,  0, -1, -2, -3 },
            { 2,  1,  2,  1,  2,  1,  2,  1 },
            {-1,  0,  1,  0,  1,  0,  1,  0 }
    };

    Matrix B{
            { 1,  2,  3,  4, -1, -2, -3, -4},
            { 2,  3,  0,  1,  0,  1,  0,  1},
            { 0,  1,  2,  3,  4,  5,  6,  7},
            {-1, -2,  1,  2, -1, -2,  1,  2},
            { 3,  4,  5,  6,  2,  3,  4,  5},
            { 0,  1,  0,  1,  0,  1,  0,  1},
            { 1,  2,  3,  4, -1, -2, -3, -4},
            { 2,  3,  0,  1,  0,  1,  0,  1},
    };

    Matrix expected{
            {42, 70, 58, 94, 1, 11, 3, 21},
            {9,	15,	17,	25,	14,	22,	28,	38},
            {22, 42, 40, 58,	18,	24,	16,	20},
            {31, 53, 45, 73, 7, 17, 13, 29},
            {8, 16, 22, 34, -5, -7, -9, -7},
            {-15, -29, -5, -11, -15, -27, -11, -15},
            {13, 23, 27, 39, 7, 9, 9, 13},
            {3, 5, 7, 9, 6, 8, 10, 12}
    };

    auto C = A * B;

    for(auto i = 0; i < 8; ++i){
        for(auto j = 0; j < 8; ++j){
            ASSERT_FLOAT_EQ(expected[i][j], C[i][j]);
        }
    }
}

TEST_F(MatrixFixture, matrixScalarMultiplication) {
    Matrix A{
            { 2,  3,  1,  4,  5,  6,  7,  8 },
            {-1,  0,  2,  1,  3,  2,  0,  1 },
            { 4,  2,  5, -1,  2,  1,  3,  2 },
            { 0,  1,  2,  3,  4,  5,  6,  7 },
            { 3,  1,  0,  2,  1,  3,  2,  0 },
            { 1, -2, -3,  4,  0, -1, -2, -3 },
            { 2,  1,  2,  1,  2,  1,  2,  1 },
            {-1,  0,  1,  0,  1,  0,  1,  0 }
    };

    auto result = 2.0f * A;

    for(int i = 0; i < 8; ++i){
        for(int j = 0; j < 8; ++j){
            ASSERT_EQ(result[i][j], A[i][j] * 2);
        }
    }
}

TEST_F(MatrixFixture, isLowerTriangularMatrix) {
    Matrix matrix{
            {1, 0, 0, 0, 0, 0, 0, 0},
            {1, 2, 0, 0, 0, 0, 0, 0},
            {1, 2, 3, 0, 0, 0, 0, 0},
            {1, 2, 3, 4, 0, 0, 0, 0},
            {1, 2, 3, 4, 5, 0, 0, 0},
            {1, 2, 3, 4, 5, 6, 0, 0},
            {1, 2, 3, 4, 5, 6, 7, 0},
            {1, 2, 3, 4, 5, 6, 7, 8},
    };

    ASSERT_TRUE(isLowerTriangular(matrix));

    matrix = Matrix{
            { 2,  3,  1,  4,  5,  6,  7,  8 },
            {-1,  0,  2,  1,  3,  2,  0,  1 },
            { 4,  2,  5, -1,  2,  1,  3,  2 },
            { 0,  1,  2,  3,  4,  5,  6,  7 },
            { 3,  1,  0,  2,  1,  3,  2,  0 },
            { 1, -2, -3,  4,  0, -1, -2, -3 },
            { 2,  1,  2,  1,  2,  1,  2,  1 },
            {-1,  0,  1,  0,  1,  0,  1,  0 }
    };

    ASSERT_FALSE(isLowerTriangular(matrix));
}

TEST_F(MatrixFixture, matrixPower) {
    Matrix A{
        {1, 2, 3},
        {4, 5, 6},
        {7, 8, 9}
    };

    auto result = pow(A, 2);

    ASSERT_EQ(result[0][0], 30);
    ASSERT_EQ(result[0][1], 36);
    ASSERT_EQ(result[0][2], 42);
    ASSERT_EQ(result[1][0], 66);
    ASSERT_EQ(result[1][1], 81);
    ASSERT_EQ(result[1][2], 96);
    ASSERT_EQ(result[2][0], 102);
    ASSERT_EQ(result[2][1], 126);
    ASSERT_EQ(result[2][2], 150);
}

TEST_F(MatrixFixture, inverseLowerTriangularMatrix) {
    Matrix  matrix{
            {1, 0, 0, 0},
            {2.5, 0.5, 0, 0},
            {0.8, 1.2, 0.25, 0},
            {0.625, 1, 3.5, 1.4},
    };

    ASSERT_TRUE(isLowerTriangular(matrix));

    auto result = lowerTriangularInvert(matrix);


    ASSERT_NEAR(result[0][0], 1, 0.001);
    ASSERT_NEAR(result[0][1], 0, 0.001);
    ASSERT_NEAR(result[0][2], 0, 0.001);
    ASSERT_NEAR(result[0][3], 0, 0.001);

    ASSERT_NEAR(result[1][0], -5, 0.001);
    ASSERT_NEAR(result[1][1], 2, 0.001);
    ASSERT_NEAR(result[1][2], 0, 0.001);
    ASSERT_NEAR(result[1][3], 0, 0.001);

    ASSERT_NEAR(result[2][0], 20.8, 0.001);
    ASSERT_NEAR(result[2][1], -9.6, 0.001);
    ASSERT_NEAR(result[2][2], 4, 0.001);
    ASSERT_NEAR(result[2][3], 0, 0.001);

    ASSERT_NEAR(result[3][0], -48.875, 0.001);
    ASSERT_NEAR(result[3][1], 22.571, 0.001);
    ASSERT_NEAR(result[3][2], -10, 0.001);
    ASSERT_NEAR(result[3][3], 0.714, 0.001);

}