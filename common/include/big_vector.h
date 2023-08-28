#pragma once

#include <vector>
#include <algorithm>
#include <initializer_list>
#include <cmath>
#include <unordered_map>
#include <cassert>

namespace {

    enum VectorType {
        Dense, Sparse
    };

    template<typename T>
    class VectorT;

    template<typename T>
    class SparseVectorT;

    template<typename T>
    class VectorT {
    public:
        static constexpr VectorType type = VectorType::Dense;

        VectorT() = default;

        VectorT(size_t size)
                : m_data(size) {}

        VectorT(std::initializer_list<T> list) {
            m_data.resize(list.size());
            std::copy(list.begin(), list.end(), m_data.begin());
        }

        VectorT(const VectorT &source) {
            m_data.resize(source.size());
            std::copy(source.cbegin(), source.cend(), m_data.begin());
        }

        VectorT(VectorT &&source) noexcept {
            source.m_data = std::exchange(m_data, source.m_data);
        }

        VectorT &operator=(const VectorT &source) {
            if (this != &source) {
                m_data.resize(source.size());
                std::copy(source.m_data.begin(), source.m_data.end(), m_data.begin());
            }
            return *this;
        }

        VectorT &operator=(VectorT &&source) noexcept {
            if (this != &source) {
                source.m_data = std::exchange(m_data, source.m_data);
            }
            return *this;
        }

        [[nodiscard]]
        size_t size() const {
            return m_data.size();
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

        T &operator[](int index) {
            return m_data[index];
        }

        const T &operator[](int index) const {
            return m_data[index];
        }

        void clear() {
            std::fill_n(m_data.begin(), m_data.size(), T{});
        }

        void resize(size_t newSize) {
            m_data.resize(newSize);
        }

        operator SparseVectorT<T>() const;

        template<typename U>
        friend VectorT<U> operator+(const VectorT<U> &v0, const VectorT<U> v1) {
            assert(v0.size() == v1.size());
            VectorT result(v0.size());
            for (int i = 0; i < v0.size(); i++) {
                result[i] = v0[i] + v1[i];
            }

            return result;
        }

        template<typename U>
        friend VectorT<U> operator-(const VectorT<U> &v0, const VectorT<U> v1) {
            assert(v0.size() == v1.size());
            VectorT result(v0.size());
            for (int i = 0; i < v0.size(); i++) {
                result[i] = v0[i] - v1[i];
            }

            return result;
        }

        template<typename U>
        friend VectorT<U> operator*(const VectorT<U> &v0, const VectorT<U> v1) {
            assert(v0.size() == v1.size());
            VectorT result(v0.size());
            for (int i = 0; i < v0.size(); i++) {
                result[i] = v0[i] * v1[i];
            }

            return result;
        }

        template<typename U>
        friend VectorT<U> operator*(const VectorT<U> &v0, U scalar) {
            VectorT result(v0.size());
            for (int i = 0; i < v0.size(); i++) {
                result[i] = v0[i] * scalar;
            }
            return result;
        }

        template<typename U>
        friend VectorT<U> operator*(U scalar, const VectorT<U> &v0) {
            return v0 * scalar;
        }

        T dot(const VectorT<T> &v1) const {
            assert(size() == v1.size());
            T result{};
            const auto &v0 = *this;
            for (int i = 0; i < size(); i++) {
                result += v0[i] * v1[i];
            }
            return result;
        }

        T dot(const SparseVectorT<T> &v1) const;

        T length() const {
            return std::sqrt(squaredLength());
        }

        inline T squaredLength() const {
            return dot(*this);
        }

    private:
        std::vector<T> m_data{};
    };


    template<typename T>
    class SparseVectorT {
    public:
        static constexpr VectorType type = VectorType::Sparse;

        struct EntryT {
            int id;
            T value;
        };

        SparseVectorT() = default;

        SparseVectorT(std::initializer_list<EntryT> list) {
            for (auto [id, value]: list) {
                m_data[id] = value;
            }
        }

        template<typename U>
        SparseVectorT(const VectorT<U> &source) {
            U zero{};
            for (int i = 0; i < source.size(); i++) {
                auto v = source[i];
                if (v != zero) {
                    m_data[i] = v;
                }
            }
        }

        SparseVectorT(const SparseVectorT &source) noexcept {
            for (auto [i, v]: source.m_data) {
                m_data[i] = v;
            }
        }

        SparseVectorT &operator=(const SparseVectorT &source) noexcept {
            if (this != &source) {
                for (auto [i, v]: source.m_data) {
                    m_data[i] = v;
                }
            }
            return *this;
        }

        const T &operator[](int index) const {
            if (m_data.contains(index)) {
                return m_data.at(index);
            }
            static const T zero{};
            return zero;
        }

        void set(EntryT entry) {
            m_data[entry.id] = entry.value;
        }

        template<typename U>
        friend VectorT<U> operator+(const VectorT<U> v0, const SparseVectorT<U> &v1) {
            return v1 + v0;
        }

        template<typename U>
        friend VectorT<U> operator+(const SparseVectorT<U> &v0, const VectorT<U> v1) {
            VectorT<U> result(v0.size());
            for (auto [i, v]: v0) {
                result[i] = v + v1[i];
            }

            return result;
        }

        template<typename U>
        friend VectorT<U> operator-(const SparseVectorT<U> &v0, const VectorT<U> v1) {
            VectorT<U> result(v0.size());
            for (auto [i, v]: v0) {
                result[i] = v - v1[i];
            }

            return result;
        }

        template<typename U>
        friend VectorT<U> operator-(const VectorT<U> v0, const SparseVectorT<U> &v1) {
            VectorT<U> result(v0.size());
            for (auto [i, v]: v1) {
                result[i] = v0[i] - v;
            }

            return result;
        }

        template<typename U>
        friend VectorT<U> operator*(const VectorT<U> &v0, const SparseVectorT<U> v1) {
            return v1 * v0;
        }

        template<typename U>
        friend VectorT<U> operator*(const SparseVectorT<U> &v0, const VectorT<U> v1) {
            VectorT<U> result(v0.size());
            for (auto [i, v]: v0) {
                result[i] = v * v1[i];
            }

            return result;
        }

        template<typename U>
        friend SparseVectorT<U> operator*(const SparseVectorT<U> &v0, U scalar) {
            auto result = v0;
            for (auto &[_, v]: result) {
                result *= scalar;
            }
            return result;
        }

        template<typename U>
        friend SparseVectorT<U> operator*(U scalar, const SparseVectorT<U> &v0) {
            return v0 * scalar;
        }

        T dot(const VectorT<T> &v1) const {
            const auto &v0 = *this;
            T result{};
            for (const auto [i, v]: v0.m_data) {
                result += v * v1[i];
            }
            return result;
        }


        T length() const {
            return std::sqrt(squaredLength());
        }

        inline T squaredLength() const {
            return dot(*this);
        }

        auto begin() noexcept {
            return m_data.begin();
        }

        auto end() noexcept {
            return m_data.end();
        }

        auto cbegin() const noexcept {
            return m_data.cbegin();
        }

        auto cend() const noexcept {
            return m_data.cend();
        }

    private:
        std::unordered_map<int, T> m_data{};
    };

    template<typename T>
    VectorT<T>::operator SparseVectorT<T>() const {
        return SparseVectorT<T>(*this);
    }

    template<typename T>
    T VectorT<T>::dot(const SparseVectorT<T> &v1) const {
        return v1.dot(*this);
    }

    using Vector = VectorT<float>;
    using SparseVector = SparseVectorT<float>;

}