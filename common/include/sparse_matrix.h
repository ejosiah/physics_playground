#pragma once

#include "big_vector.h"
#include <tuple>
#include <unordered_map>
#include <initializer_list>
#include <iostream>

namespace blas {

    template<typename T>
    class MatrixT;

    template<typename T>
    class SparseMatrixT {
        using Size = std::tuple<size_t, size_t>;
        using Coltype = SparseVectorT<T>;
    public:
        SparseMatrixT(size_t size) : m_data(size) {}

        SparseMatrixT(std::initializer_list<Coltype> list) {
            m_data.resize(list.size());
            std::copy(list.begin(), list.end(), m_data.begin());
        }

        template<typename U>
        explicit SparseMatrixT(const MatrixT<U>& source);

        Coltype &operator[](int index) {
            return m_data[index];
        }

        const Coltype &operator[](int index) const {
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

        [[nodiscard]]
        Size size() const {
            return std::make_tuple(m_data.size(), m_data.size());
        }

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
        std::vector<Coltype> m_data{};
    };

    using SparseMatrix = SparseMatrixT<float>;

}