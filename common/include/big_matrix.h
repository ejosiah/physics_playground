#pragma once

#include "big_vector.h"

template<typename T>
class BigMatrixT {
    using Size = std::tuple<size_t, size_t>;
    using Coltype = BigVectorT<T>;
public:
    BigMatrixT(size_t rows, size_t columns)
    : m_data(rows)
    {
        std::fill_n(m_data.begin(), Coltype(columns));
    }

    BigMatrixT(std::initializer_list<Coltype> list)
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

    [[nodiscard]]
    Size size() const {
        return std::make_tuple(m_data[0].size(), m_data.size());
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
    friend BigVectorT<U> operator*(const BigMatrixT<U>& matrix, const BigVectorT<U>& v) {

        auto [cols, rows] = matrix.size();
        assert(cols == v.size());
        BigVectorT<U> result(cols);

        for(int i = 0; i < cols; i++){
            result[i] = v.dot(matrix[i]);
        }
        return result;
    }

private:
    std::vector<Coltype> m_data{};
};

using BigMatrix = BigMatrixT<float>;