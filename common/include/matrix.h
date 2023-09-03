#pragma once

#include "big_matrix.h"
#include "sparse_matrix.h"
#include <string_view>
#include <fstream>
#include <format>
#include <stdexcept>
#include <iostream>


template<typename T>
blas::SparseMatrixT<T>::SparseMatrixT(const MatrixT<T> &source) {
    auto [_, rows] = source.size();
    m_data.resize(rows);
    for(auto i = 0; i < rows; i++){
        m_data[i] = source[i];
    }
}

//template<typename T>
//blas::MatrixT<T>::MatrixT(const SparseMatrixT<T> &source)
//: blas::MatrixT<T>::MatrixT(source.rows())
//{
//    const auto [cols, rows] = size();
//    const auto& sm = *this;
//    for(auto i = 0; i < rows; i++){
//        for(auto j = 0; j < cols; j++){
//            m_data[i][j] = sm[i][j];
//        }
//    }
//}

template<typename T>
blas::SparseMatrixT<T>::operator blas::MatrixT<T>() const {
    const auto [cols, rows] = size();
    MatrixT<T> matrix(cols, rows);
    const auto& sm = *this;
    for(auto i = 0; i < rows; i++){
        for(auto j = 0; j < cols; j++){
            matrix[i][j] = sm[i][j];
        }
    }
    return matrix;
}

template<typename Matrix>
Matrix pow(const Matrix& matrix, int exponent) {
    auto [_, rows] = matrix.size();
    Matrix result(rows, rows);
    result.identity();
    for(auto i = 0; i < exponent; i++){
        result = result * matrix;
    }

    return result;
}

bool isLowerTriangular(const auto& matrix) {
    auto [cols, rows] = matrix.size();

    for(auto i = 0; i < rows; ++i) {
        for(auto j = i + 1; j < cols; ++j){
            if(matrix[i][j] != 0) return false;
        }
    }
    return true;
}

template<typename T>
blas::SparseMatrixT<T> load(std::string_view mtxFile) {
    std::ifstream fin{mtxFile.data()};
    if(fin.bad()){
        throw std::runtime_error{std::format("unable to load mtx file {}", mtxFile)};
    }
    size_t rows, cols, size;
    fin >> rows >> cols >> size;
    std::cout << std::format("load sparse matrix with {} rows, {} cols and {} data points\n", rows, cols, size);

    blas::SparseMatrixT<T> matrix(rows);
    for(auto k = 0; k < size; ++k){
        int row, col;
        T entry{};
        fin >> row >> col >> entry;
        matrix[row - 1].set({col - 1, entry});
    }
    return matrix;

}

template<typename Matrix>
Matrix lowerTriangularInvert(const Matrix& matrix) {
    const auto [cols, rows] = matrix.size();
    assert(isLowerTriangular(matrix));
    assert(cols == rows);


    Matrix D(rows, cols);

    for(int i = 0; i < rows; ++i) {
        D[i][i] = matrix[i][i];
    }
    auto N = matrix - D;

    for(int j = 0; j < cols; ++j) {
        for(auto i = j+1; i < rows; ++i){
            N[i][j] /= D[i][i];
        }
    }


    Matrix Ninv(rows, cols);
    for(int k = 1; k < rows; ++k) {
        Ninv = Ninv + std::pow(-1, k) * pow(N, k);
    }

    auto QInv = Matrix(rows, cols).identity() + Ninv;


    Matrix DInv(rows, cols);
    auto t = DInv[0][0];
    for(auto i = 0; i < rows; i++){
        DInv[i][i] = decltype(t){1}/D[i][i];
    }

    return QInv * DInv;
}