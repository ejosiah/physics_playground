#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <tuple>
#include <iterator>
#include <cstddef>
#include <span>
#include "types.h"

enum class Field : int { Position = 0, Velocity};

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
};


template<typename ValueType, Field field, typename Layout>
class View{
public:
    View(Layout layout)
    : m_layout{layout}
    , m_offset(layout.get(field))
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
        int padding{};
    };

    using Members = MembersType;

    Members* data{};
    size_t size{};

    [[nodiscard]]
    int get(Field field) const {
        constexpr auto width = sizeof(Members);
        switch(field){
            case Field::Position: return offsetof(Members, position);
            case Field::Velocity: return offsetof(Members, velocity);
        }
    }
};


template<glm::length_t L>
using InterleavedMemoryParticles = Particles<L, InterleavedMemoryLayout>;

using InterleavedMemoryParticle2D = Particles<2, InterleavedMemoryLayout>;

template<glm::length_t L>
InterleavedMemoryParticles<L> createInterleavedMemoryParticle(std::span<typename InterleavedMemoryParticles<L>::Layout::Members> span) {
    return { { span.data(), span.size() } };
}