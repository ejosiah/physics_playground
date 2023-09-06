#pragma once

#include "matrix.h"
#include <iostream>

blas::SparseMatrix generateMatrix(size_t size, float c = 1) {
     blas::SparseMatrix sm(size);

    sm[0][0] = 2 * c + 1;
    sm[0][1] = -c;

     const int tail = static_cast<int>(size - 1);
    sm[tail][tail - 1] = -c;
    sm[tail][tail] = 2 * c + 1;
     for(int i = 1; i < tail; i++){
         sm[i][i - 1] = -c;
         sm[i][i] = 2 * c + 1;
         sm[i][i + 1] = -c;
     }

     return sm;
}

/**
 *
 * @param M - number of rows in grid
 * @param N - number of columns in grid
 * @param scale - scaling factor
 * @return
 */
blas::SparseMatrix generatePoissonEquationMatrix(size_t M, size_t N, float scale = 1) {
    blas::SparseMatrix m(M * N);

    for(int r = 0; r < M; r++){
        for(int c = 0; c < N; c++){
            auto left = c - 1;
            auto right = c + 1;
            auto top = r + 1;
            auto bottom = r - 1;
            int i = r * N + c;

            float s = 4.0;
            if(left >= 0){
                m[i][i-1] = -scale;
            }else{
                s -= 1;
            }

            if(right < N){
                m[i][i+1] = -scale;
            }else {
                s -= 1;
            }

            if(bottom >= 0){
                m[i][bottom * N + c] = -scale;
            }else {
                s -= 1;
            }

            if(top < M){
                m[i][top * N + c] = -scale;
            }else {
                s -= 1;
            }

            m[i][i] = s * scale + 1;
        }
    }
    return m;
}

blas::SparseMatrix generatePoissonEquationMatrix(size_t N, float scale = 1) {
    return generatePoissonEquationMatrix(N, N, scale);
}