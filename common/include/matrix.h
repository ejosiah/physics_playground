#pragma once

#include "big_matrix.h"
#include "sparse_matrix.h"

template<typename T>
template<typename U>
blas::SparseMatrixT<T>::SparseMatrixT(const MatrixT<U> &source) {
    auto [_, rows] = source.size();
    m_data.resize(rows);
    for(auto i = 0; i < rows; i++){
        m_data[i] = source[i];
    }
}