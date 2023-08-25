#pragma once

#include <tuple>
#include <unordered_map>
#include <initializer_list>
#include "big_vector.h"

template<typename T>
class SparseMatrixT {
    using Size = std::tuple<size_t, size_t>;
    using Coltype = SparseVectorT<T>;
public:
    SparseMatrixT(size_t size) : m_data(size)
    {}

    SparseMatrixT(std::initializer_list<Coltype> list)
    {
        m_data.resize(list.size());
        std::copy(list.begin(), list.end(), m_data.begin());
    }

    Coltype& operator[](int index){
        return m_data[index];
    }

    const Coltype& operator[](int index) const {
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
    friend BigVectorT<U> operator*(const SparseMatrixT<U>& matrix, const BigVectorT<U>& v) {

        auto [cols, rows] = matrix.size();
        assert(cols == v.size());
        BigVectorT<U> result(cols);

        for(int i = 0; i < cols; i++){
            result[i] = v.dot(matrix[i]);
        }
        return result;
    }

    Size size() const {
        return std::make_tuple(m_data.size(), m_data.size());
    }

private:
    std::vector<Coltype> m_data{};
};

using SparseMatrix = SparseMatrixT<float>;