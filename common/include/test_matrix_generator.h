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