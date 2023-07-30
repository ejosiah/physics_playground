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

template<glm::length_t L, template<typename> typename LayoutType>
struct Particles{

    using VecType = glm::vec<L, float>;
    using Layout = LayoutType<VecType>;

    Layout layout;
    size_t size;

    auto position() {
        return layout.position();
    }

    auto velocity() {
        return layout.velocity();
    }

    auto inverseMass() {
        return layout.inverseMass();
    }

    auto restitution() {
        return layout.restitution();
    }
    auto radius() {
        return layout.radius();
    }

};

template<typename VecType>
struct InterleavedMemoryLayout {
    using Vec = VecType;

    template<typename ValueType, Field field>
    class View{
    public:

        View(InterleavedMemoryLayout layout)
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

    private:
        InterleavedMemoryLayout m_layout{};
        int m_offset{0};
    };

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

    auto position() {
        return View<VecType, Field::Position>{ *this };
    }

    auto velocity() {
        return View<VecType, Field::Velocity>{ *this };
    }

    auto inverseMass() {
        return View<float, Field::Mass>{ *this };
    }

    auto restitution() {
        return View<float, Field::Restitution>{ *this };
    }
    auto radius() {
        return View<float, Field::Radius>{ *this };
    }
};

template<typename VecType>
struct SeparateFieldMemoryLayout {
    using Vec = VecType;

    struct MemberType{
        std::span<Vec> position{};
        std::span<Vec> prePosition{};
        std::span<Vec> velocity{};
        std::span<float> inverseMass{};
        std::span<float> restitution{};
        std::span<float> radius{};
    };

    using Members = MemberType;
    static constexpr auto Width = sizeof(Vec) * 3 + sizeof(float) * 3;

    Members data{};

    template<typename ValueType, Field field>
    class View{
    public:

        View(SeparateFieldMemoryLayout layout)
                : m_layout{layout}
                , m_ptr(layout.get<ValueType, field>())
        {}

        ValueType& operator[](int id) {
            return m_ptr[id];
        }

        ValueType& operator[](int id) const {
            return m_ptr[id];
        }

    private:
        SeparateFieldMemoryLayout m_layout{};
        ValueType* m_ptr{0};
    };



    template<typename ValueType, Field field>
    [[nodiscard]]
    constexpr ValueType* get() const {
        switch(field){
            case Field::Position: return as<ValueType>(data.position.data());
            case Field::Velocity: return as<ValueType>(data.velocity.data());
            case Field::Mass: return as<ValueType>(data.inverseMass.data());
            case Field::Restitution: return as<ValueType>(data.restitution.data());
            case Field::Radius: return as<ValueType>(data.radius.data());
        }
        throw std::runtime_error{ "invalid field" };
    }

    auto position() {
        return View<VecType, Field::Position>{ *this };
    }

    auto velocity() {
        return View<VecType, Field::Velocity>{ *this };
    }

    auto inverseMass() {
        return View<float, Field::Mass>{ *this };
    }

    auto restitution() {
        return View<float, Field::Restitution>{ *this };
    }

    auto radius() {
        return View<float, Field::Radius>{ *this };
    }

};

using InterleavedMemoryLayout2D = InterleavedMemoryLayout<glm::vec2>;
using SeparateFieldMemoryLayout2D = SeparateFieldMemoryLayout<glm::vec2>;

template<template<typename> typename Layout>
using Particle2D = Particles<2, Layout>;

template<glm::length_t L>
using InterleavedMemoryParticles = Particles<L, InterleavedMemoryLayout>;

using InterleavedMemoryParticle2D = Particles<2, InterleavedMemoryLayout>;

template<glm::length_t L>
using SeparateFieldParticles = Particles<L, SeparateFieldMemoryLayout>;

using SeparateFieldParticle2D = Particles<2, SeparateFieldMemoryLayout>;

template<glm::length_t L>
inline InterleavedMemoryParticles<L> createInterleavedMemoryParticle(std::span<typename InterleavedMemoryParticles<L>::Layout::Members> span) {
    return { { span.data(), span.size() } };
}

inline InterleavedMemoryParticle2D createInterleavedMemoryParticle2D(std::span<typename InterleavedMemoryParticle2D::Layout::Members> span) {
    return { { span.data(), span.size() } };
}

inline SeparateFieldParticle2D createSeparateFieldParticle2D(std::span<glm::vec2> positions
                                                             , std::span<glm::vec2> velocity
                                                             , std::span<float> inverseMass
                                                             , std::span<float> restitution
                                                             , std::span<float> radius) {
    return { { positions, positions, velocity, inverseMass, restitution, radius }};
}