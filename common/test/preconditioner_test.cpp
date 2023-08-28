#include "preconditioner.h"
#include "matrix.h"
#include <gtest/gtest.h>
#include <iostream>

using namespace blas;

TEST(preconditioner, preconditionDenseMatrix) {
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

    auto M = precondition(A);

    std::cout << A << "\n\n";
    std::cout << M << "\n\n";
}