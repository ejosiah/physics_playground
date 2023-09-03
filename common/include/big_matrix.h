#pragma once

#include "blas_forwards.h"
#include "big_vector.h"
#include <iostream>

namespace blas {

    enum class InversionMethod { LowerTriangular };

    template<typename T>
    class MatrixT {
        using Size = std::tuple<size_t, size_t>;
        using RowType = VectorT<T>;
        using Coltype = VectorT<T>;
        static constexpr VectorType Layout = VectorType::Dense;
    public:
        MatrixT(size_t size) : MatrixT(size, size) {}

        MatrixT(size_t rows, size_t columns)
                : m_data(rows) {
            std::fill(m_data.begin(), m_data.end(), RowType(columns));
        }

        MatrixT(std::initializer_list<RowType> list) {
            m_data.resize(list.size());
            std::copy(list.begin(), list.end(), m_data.begin());
        }

//        MatrixT(const SparseMatrixT<T>& source);

        MatrixT(const MatrixT<T>& source)
        :m_data(source.m_data.size()){
            std::copy(source.m_data.cbegin(), source.m_data.end(), m_data.begin());
        }

        MatrixT<T>& operator=(const MatrixT<T>& source) {
            if(this != &source){
                m_data.resize(source.m_data.size());
                std::copy(source.m_data.begin(), source.m_data.end(), m_data.begin());
            }

            return *this;
        }

        RowType &operator[](int index) {
            return m_data[index];
        }

        const RowType &operator[](int index) const {
            return m_data[index];
        }

        RowType row(int index) const {
            return m_data[index];
        }

        Coltype column(int index)  const {
            const auto [_, rows] = size();
            Coltype result(rows);
            for(int i = 0; i < rows; i++) {
                result[i] = m_data[i][index];
            }
            return result;
        }


        [[nodiscard]]
        Size size() const {
            return std::make_tuple(m_data[0].size(), m_data.size());
        }

        [[nodiscard]]
        size_t rows() const {
            return m_data.size();
        }

        [[nodiscard]]
        size_t columns() const {
            return m_data[0].size();
        }

        auto begin() {
            return m_data.begin();
        }

        auto end() {
            return m_data.end();
        }

        auto cbegin() const {
            return m_data.cbegin();
        }

        auto cend() const {
            return m_data.cend();
        }

        template<typename U>
        friend VectorT<U> operator*(const MatrixT<U> &matrix, const VectorT<U> &v) {

            auto [cols, rows] = matrix.size();
            assert(cols == v.size());
            VectorT<U> result(cols);

            for (int i = 0; i < cols; i++) {
                result[i] = v.dot(matrix[i]);
            }
            return result;
        }

        template<typename U>
        friend std::ostream& operator<<(std::ostream& out, const MatrixT<U> matrix) {
            auto [cols, rows] = matrix.size();
            for(auto i = 0; i < rows; i++){
                for(auto j = 0; j < cols; j++){
                    out << matrix[i][j] << ", ";
                }
                out << "\n";
            }

            return out;
        }

        template<typename U>
        friend MatrixT<U> transpose(const MatrixT<U>& matrix){
            const auto [cols, rows] = matrix.size();
            MatrixT<U> result(rows, cols);

            for(auto i = 0; i < rows; i++){
                for(auto j = 0; j < cols; j++){
                    result[i][j] = matrix[j][i];
                }
            }
            return result;
        }

        MatrixT<T>& identity() {
            for(auto& row : m_data){
                row.clear();
            }
            for(auto i = 0; i < rows(); i++){
                m_data[i][i] = T{1};
            }
            return *this;
        }


        template<typename U>
        friend MatrixT<U> operator+(const MatrixT<U>& A, const MatrixT<U>& B) {
            const auto [cols, rows] = A.size();
            MatrixT<U> result = A;

            for(auto i = 0; i < rows; i++){
                for(auto j = 0; j < cols; j++) {
                    result[i][j] += B[i][j];
                }
            }

            return result;
        }

        template<typename U>
        friend MatrixT<U> operator-(const MatrixT<U>& A, const MatrixT<U>& B) {
            const auto [cols, rows] = A.size();
            MatrixT<U> result = A;

            for(auto i = 0; i < rows; i++){
                for(auto j = 0; j < cols; j++) {
                    result[i][j] -= B[i][j];
                }
            }

            return result;
        }

        template<typename U>
        friend MatrixT<U> operator*(const MatrixT<U>& A, const MatrixT<U>& B){
            auto [cols, _1] = A.size();
            auto [_2, rows] = B.size();
            assert(cols == rows);

            MatrixT<U> result(rows, cols);
            for(auto i = 0; i < rows; ++i) {
                const auto v0 = A.row(i);
                for(auto j = 0; j < cols; ++j) {
                    const auto v1 = B.column(j);
                    auto res0 = v0.dot(v1);
                    result[i][j] = res0;
                }
            }
            return result;
        }

        template<typename U, typename S>
        friend MatrixT<U> operator*(S scalar, const MatrixT<U>& A) {
            return A * scalar;
        }

        template<typename U, typename S>
        friend MatrixT<U> operator*(const MatrixT<U>& A, S scalar) {
            const auto [cols, rows] = A.size();
            MatrixT<U> result = A;

            for(auto i = 0; i < rows; i++){
                for(auto j = 0; j < cols; j++) {
                    result[i][j] *= scalar;
                }
            }

            return result;
        }

    private:
        std::vector<RowType> m_data{};
    };

    using Matrix = MatrixT<float>;


}