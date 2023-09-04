#pragma once

#include "blas_forwards.h"
#include <vector>
#include <algorithm>
#include <initializer_list>
#include <cmath>
#include <unordered_map>
#include <cassert>
#include <memory>
#include <iterator>

namespace blas {

    enum VectorType {
        Dense, Sparse
    };

    template<typename T>
    class VectorT {
    public:
        static constexpr VectorType type = VectorType::Dense;

    class iterator : public std::iterator<
                                std::random_access_iterator_tag,
                                T,
                                std::ptrdiff_t,
                                const T*,
                                const T&>{

    public:
        iterator(T* start)
        : m_ptr{start}
        {}

        iterator& operator++() noexcept {
            ++m_ptr;
            return *this;
        }

        iterator& operator--() noexcept {
            --m_ptr;
            return *this;
        }

        iterator& operator++() const noexcept {
            ++m_ptr;
            return *this;
        }

        iterator& operator--() const noexcept {
            --m_ptr;
            return *this;
        }

        iterator operator++(int) noexcept {
            iterator retval = *this;
            ++(*this);
            return retval;
        }


        iterator operator--(int) noexcept {
            iterator retval = *this;
            --(*this);
            return retval;
        }

        iterator operator++(int) const noexcept {
            iterator retval = *this;
            ++(*this);
            return retval;
        }


        iterator operator--(int) const noexcept {
            iterator retval = *this;
            --(*this);
            return retval;
        }

        bool operator==(iterator other) const noexcept {return m_ptr == other.m_ptr;}

        bool operator!=(iterator other) const noexcept {return !(*this == other);}

        const T& operator*() const noexcept { return *m_ptr; }

        T& operator*()  noexcept { return *m_ptr; }

        const T* operator->() const noexcept {
            return m_ptr;
        }

        T* operator->() noexcept {
            return m_ptr;
        }

        iterator& operator+=(long offset) noexcept {
            m_ptr += offset;
            return *this;
        }

        iterator& operator-=(long offset) noexcept {
            m_ptr -= offset;
            return *this;
        }

        iterator& operator+=(long offset) const noexcept {
            m_ptr += offset;
            return *this;
        }

        iterator& operator-=(long offset) const noexcept {
            m_ptr -= offset;
            return *this;
        }

        ptrdiff_t operator-(iterator other) const noexcept {
            return reinterpret_cast<char*>(m_ptr) - reinterpret_cast<char*>(other.m_ptr);
        }


    private:
        mutable T* m_ptr;
        long m_offset{0};
    };

        VectorT() = default;

        explicit VectorT(size_t size)
                : m_size{ size }
                , m_data{new T[size] } {
            clear();
        }

        VectorT(T* allocation, size_t size)
        : m_size{ size }
        , m_data{ allocation }
        , owned{ false }
        {}

       ~VectorT() noexcept {
            if(owned && m_data){
                delete[] m_data;
            }
        }

        VectorT(std::initializer_list<T> list)
        :VectorT(list.size()){
            std::copy(list.begin(), list.end(), begin());
        }

        VectorT(const VectorT &source)
        :VectorT(source.m_size){
            std::copy(source.cbegin(), source.cend(), begin());
        }

        VectorT(VectorT &&source) noexcept {
            source.m_data = std::exchange(m_data, source.m_data);
            source.m_size = std::exchange(m_size, source.m_size);
        }

        VectorT &operator=(const VectorT &source) {
            if (this != &source) {
                m_size = source.m_size;
                m_data = new T[m_size];
                std::copy(source.cbegin(), source.cend(), begin());
            }
            return *this;
        }

        VectorT &operator=(VectorT &&source) noexcept {
            if (this != &source) {
                source.m_data = std::exchange(m_data, source.m_data);
                source.m_size = std::exchange(m_size, source.m_size);
            }
            return *this;
        }

        [[nodiscard]]
        size_t size() const {
            return m_size;
        }

        auto begin() {
            return iterator{ m_data };
        }

        auto end() {
            return iterator{ m_data + m_size};
        }

        auto cbegin() const {
            return iterator{ m_data };
        }

        auto cend() const {
            return iterator{ m_data + m_size};
        }

        T &operator[](int index) {
            return m_data[index];
        }

        const T &operator[](int index) const {
            return m_data[index];
        }

        void clear() {
            std::fill_n(begin(), m_size, T{});
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
        T* m_data{};
        size_t m_size{};
        bool owned{true};
        std::vector<T> v;
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

        T &operator[](int index)  {
            if (!m_data.contains(index)) {
                m_data[index] = 0;
            }
           return m_data[index];
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

        void clear() {
            m_data.clear();
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