#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <tuple>
#include <iterator>
#include <cstddef>
#include <span>
#include "types.h"
#include <stdexcept>

enum class Field : int { Position = 0, Velocity, Mass, Restitution, Radius };

using OffsetStride = std::tuple<int, int>;

template<typename T>
struct Iterator : public std::iterator<
                           std::random_access_iterator_tag,
                           T,
                           long,
                           T*,
                           T>
{
    T* ptr{};
    int stride{1};

    T get() const              { return *ptr; }
    T operator*() const        { return *ptr; }
    T& operator*()             { return *ptr; }

    Iterator& operator++()        { ptr += stride; return *this; }
    Iterator operator++ (int)     { auto old = ptr; ptr += stride; return { old, stride }; }

    Iterator& operator--()        { ptr -= stride; return *this; }
    Iterator operator-- (int)     { auto old = ptr; ptr -= stride; return { old, stride }; }

    Iterator& operator+=(long offset)        { ptr += offset * stride; return *this; }
    Iterator& operator-=(long offset)        { ptr -= offset * stride; return *this; }

    template<typename U>
    bool operator !=(Iterator<U> other){
        return ptr != other.ptr;
    }
};


template<typename ValueType, Field field, typename Layout>
class View{
public:

    using iterator = Iterator<ValueType>;

    View(Layout layout)
    : m_layout{layout}
    , m_offset(layout.get<field>())
    {}

    ValueType& operator[](int id) {
        auto ptr = (m_layout.data + id);
        return *as<ValueType>(as<char>(ptr) + m_offset);
    }

    ValueType& operator[](int id) const {
        auto ptr = (m_layout.data + id);
        return *as<ValueType>(as<char>(ptr) + m_offset);
    }

    iterator begin() {
        ValueType* ptr = &this->operator[](0);
        iterator itr;
        itr.ptr = ptr;
        itr.stride = 1;

        return itr;
    }

    iterator end() {
        ValueType* ptr = (&this->operator[](m_layout.size - 1)) + 1;
        iterator itr;
        itr.ptr = ptr;
        itr.stride = 1;

        return itr;
    }

private:
    Layout m_layout{};
    int m_offset{0};

};


template<glm::length_t L, template<typename> typename LayoutType>
struct Particles{

    using VecType = glm::vec<L, float>;
    using Layout = LayoutType<VecType>;

    Layout data;
    size_t size;

    auto position() {
        return View<VecType, Field::Position, Layout>{ data };
    }

    auto velocity() {
        return View<VecType, Field::Velocity, Layout>{ data };
    }

    auto inverseMass() {
        return View<float, Field::Mass, Layout>{ data };
    }

    auto restitution() {
        return View<float, Field::Restitution, Layout>{ data };
    }
    auto radius() {
        return View<float, Field::Radius, Layout>{ data };

    }


};

template<typename VecType>
struct InterleavedMemoryLayout {
    using Vec = VecType;

    struct MembersType {
        Vec position{};
        Vec prePosition{};
        Vec velocity{};
        float inverseMass{};
        float restitution{1};
        float radius{1};
        int padding0{};
        int padding1{};
        int padding2{};
    };

    using Members = MembersType;
    static constexpr auto Width = sizeof(Members) ;

    Members* data{};
    size_t size{};

    template<Field field>
    [[nodiscard]]
    constexpr int get() const {
        switch(field){
            case Field::Position: return offsetof(Members, position);
            case Field::Velocity: return offsetof(Members, velocity);
            case Field::Mass: return offsetof(Members, inverseMass);
            case Field::Restitution: return offsetof(Members, restitution);
            case Field::Radius: return offsetof(Members, radius);
        }
        throw std::runtime_error{ "invalid field" };
    }
};

using InterleavedMemoryLayout2D = InterleavedMemoryLayout<glm::vec2>;

template<template<typename> typename Layout>
using Particle2D = Particles<2, Layout>;

template<glm::length_t L>
using InterleavedMemoryParticles = Particles<L, InterleavedMemoryLayout>;

using InterleavedMemoryParticle2D = Particles<2, InterleavedMemoryLayout>;

template<glm::length_t L>
inline InterleavedMemoryParticles<L> createInterleavedMemoryParticle(std::span<typename InterleavedMemoryParticles<L>::Layout::Members> span) {
    return { { span.data(), span.size() } };
}

inline InterleavedMemoryParticle2D createInterleavedMemoryParticle2D(std::span<typename InterleavedMemoryParticle2D::Layout::Members> span) {
    return { { span.data(), span.size() } };
}