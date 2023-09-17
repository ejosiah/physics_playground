#pragma once

#include "blas_forwards.h"
#include "big_vector.h"
#include <tuple>
#include <unordered_map>
#include <initializer_list>
#include <iostream>

namespace blas {

    template<typename T>
    class SparseMatrixT {
        using Size = std::tuple<size_t, size_t>;
        using Rowtype = SparseVectorT<T>;
    public:
        static constexpr VectorType Layout = VectorType::Sparse;

        SparseMatrixT(size_t size) : m_data(size) {}

        SparseMatrixT(size_t rows, size_t cols) : m_data(rows) {}

        SparseMatrixT(std::initializer_list<Rowtype> list) {
            m_data.resize(list.size());
            std::copy(list.begin(), list.end(), m_data.begin());
        }

        explicit SparseMatrixT(const MatrixT<T>& source);

        Rowtype &operator[](int index) {
            return m_data[index];
        }

        const Rowtype &operator[](int index) const {
            return m_data[index];
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

        void set(int i, int j, T value) {
            m_data[i][j] = value;
        }

        T get(int i, int j) const {
           return m_data[i][j];
        }

        operator MatrixT<T>() const;

        SparseMatrixT<T>& identity() {
            for(auto& row : m_data){
                row.clear();
            }
            for(auto i = 0; i < rows(); i++){
                m_data[i].set({i, T{1}});
            }
            return *this;
        }

        template<typename U>
        friend VectorT<U> operator*(const SparseMatrixT<U> &matrix, const VectorT<U> &v) {
            auto [cols, rows] = matrix.size();
            assert(cols == v.size());
            VectorT<U> result(cols);

            for (int i = 0; i < cols; i++) {
                result[i] = v.dot(matrix[i]);
            }
            return result;
        }

        template<typename U>
        friend SparseMatrixT<U> operator-(const SparseMatrixT<U>& a, const SparseMatrixT<U>& b){
            assert(a.rows() == b.rows());
            const auto size = a.rows();

            SparseMatrixT<U> c{size};
            for(auto i = 0; i < a.rows(); i++){
                c[i] = a[i] - b[i];
            }
            return c;
        }

        template<typename U>
        friend SparseMatrixT<U> operator+(const SparseMatrixT<U>& a, const SparseMatrixT<U>& b){
            assert(a.rows() == b.rows());
            const auto size = a.rows();

            SparseMatrixT<U> c{size};
            for(auto i = 0; i < a.rows(); i++){
                c[i] = a[i] + b[i];
            }
            return c;
        }

        [[nodiscard]]
        inline Size size() const {
            return std::make_tuple(m_data.size(), m_data.size());
        }

        [[nodiscard]]
        inline size_t rows() const {
            return m_data.size();
        };

        template<typename U>
        friend std::ostream& operator<<(std::ostream& out, const SparseMatrixT<U> matrix) {
            auto [cols, rows] = matrix.size();
            for(auto i = 0; i < rows; i++){
                for(auto j = 0; j < cols; j++){
                    out << matrix[i][j] << ", ";
                }
                out << "\n";
            }

            return out;
        }

    private:
        mutable std::vector<Rowtype> m_data{};
    };

    using SparseMatrix = SparseMatrixT<float>;

}