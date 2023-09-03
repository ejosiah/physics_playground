#pragma once

#include "big_vector.h"

#include <cmath>
/**
 *
 * Preconditions a Matrix using the Incomplete Cholesky factorization
 *  @see <a href="https://en.wikipedia.org/wiki/Incomplete_Cholesky_factorization">Incomplete Cholesky factorization</a>
 * @tparam A Matrix parameter type
 * @param a Matrix parameter to precondition
 * @return preconditioned Matrix
 */
template<typename A>
A precondition(A a) {
    const auto [_, N] = a.size();

    for(auto k = 0; k < N; ++k){
        a[k][k] *= a[k][k];
        for(auto i = k+1; i < N; ++i) {
            if(a[i][k] != 0){
                a[i][k] = a[i][k]/a[k][k];
            }
        }

        for(auto j = k+1; j < N; ++j) {
            for(auto i = j; i < N; ++i){
                if(a[i][j] != 0) {
                    a[i][j] = a[i][j] - a[i][k] * a[j][k];
                }
            }
        }
    }

    if constexpr (A::Layout == blas::VectorType::Dense) {
        for (auto i = 0; i < N; ++i) {
            for (auto j = i + 1; j < N; ++j) {
                a[i][j] = 0;
            }
        }
    }

    return a;
}