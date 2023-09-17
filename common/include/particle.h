#pragma once

#include "types.h"
#include <glm/glm.hpp>
#include <yaml-cpp/yaml.h>
#include <memory>
#include <tuple>
#include <iterator>
#include <span>
#include <vector>
#include <stdexcept>
#include <cstddef>
#include <memory>

enum class Field : int { Position = 0, PreviousPosition, Velocity, Mass, Restitution, Radius };

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
struct Particles {

    using VecType = glm::vec<L, float>;
    using Layout = LayoutType<VecType>;

    using Members = Layout::Members;

    using Position = Layout::template View<VecType, Field::Position>;
    using PreviousPosition = Layout::template View<VecType, Field::PreviousPosition>;
    using Velocity = Layout::template View<VecType, Field::Velocity>;
    using InverseMass = Layout::template View<float, Field::Mass>;
    using Restitution = Layout::template View<float, Field::Restitution>;
    using Radius = Layout::template View<float, Field::Radius>;

    Layout layout;

    auto position() const {
        return layout.position(_internal.seekHead);
    }

    auto previousPosition() const  {
        return layout.previousPosition(_internal.seekHead);
    }

    auto velocity() const  {
        return layout.velocity(_internal.seekHead);
    }

    auto inverseMass() const  {
        return layout.inverseMass(_internal.seekHead);
    }

    auto restitution() const  {
        return layout.restitution(_internal.seekHead);
    }

    auto radius() const {
        return layout.radius(_internal.seekHead);
    }

    auto size() const {
        return _internal.seekHead;
    }

    auto empty() const {
        return _internal.m_seekHead <= 0;
    }

    auto capacity() const {
        return layout.capacity();
    };

    void add(VecType pos, VecType vel, float invMass, float radius, float restitution) {
        layout.add(pos, pos - pos * vel * 1e-4f, vel, invMass, radius, restitution, _internal.seekHead);
        _internal.seekHead++;
    }

    struct {
        friend class Particles;
    private:
        size_t seekHead{};
    } _internal{};
};

template<typename VecType>
struct InterleavedMemoryLayout {
    using Vec = VecType;

    template<typename ValueType, Field field>
    class View{
    public:

        View(InterleavedMemoryLayout layout, size_t size = std::numeric_limits<size_t>::max())
                : m_layout{layout}
                , m_offset(layout.get<field>())
                , m_size{ size }
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
        int m_offset{};
        size_t m_size{};
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
    size_t _capacity{};

    template<Field field>
    [[nodiscard]]
    constexpr int get() const {
        switch(field){
            case Field::Position: return offsetof(Members, position);
            case Field::PreviousPosition: return offsetof(Members, prePosition);
            case Field::Velocity: return offsetof(Members, velocity);
            case Field::Mass: return offsetof(Members, inverseMass);
            case Field::Restitution: return offsetof(Members, restitution);
            case Field::Radius: return offsetof(Members, radius);
        }
        throw std::runtime_error{ "invalid field" };
    }

    auto position(size_t size) const {
        return View<VecType, Field::Position>{ *this, size };
    }

    auto previousPosition(size_t size) const {
        return View<VecType, Field::PreviousPosition>{ *this, size };

    }

    auto velocity(size_t size) const {
        return View<VecType, Field::Velocity>{ *this, size };
    }

    auto inverseMass(size_t size) const {
        return View<float, Field::Mass>{ *this, size };
    }

    auto restitution(size_t size) const {
        return View<float, Field::Restitution>{ *this, size };
    }

    auto radius(size_t size) const {
        return View<float, Field::Radius>{ *this, size };
    }

    void add(VecType pos, VecType prevPos, VecType vel, float invMass, float radius, float restitution, int index) {
        assert(index >= 0 && index < _capacity);
        data[index].position = pos;
        data[index].prePosition = prevPos;
        data[index].velocity = vel;
        data[index].inverseMass = invMass;
        data[index].radius = radius;
        data[index].restitution = restitution;
    }

    [[nodiscard]] size_t capacity() const {
        return _capacity;
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
    static constexpr auto Width = (sizeof(Vec) + sizeof(float)) * 3;

    Members data{};
    SeparateFieldMemoryLayout() = default;

    SeparateFieldMemoryLayout(std::span<Vec> position
            , std::span<Vec> prePosition
            , std::span<Vec> velocity
            , std::span<float> inverseMass
            , std::span<float> restitution
            , std::span<float> radius
    )
    {
        data = Members { position, prePosition, velocity, inverseMass, restitution, radius};
    }

    SeparateFieldMemoryLayout(size_t capacity)
    {
        int offset = 0;
        memory.resize(capacity * Width);
        allocate(memory);
    }

    SeparateFieldMemoryLayout(std::span<char> memory){
        assert(!memory.empty() && memory.size() % Width == 0);
        allocate(memory);
    }

    void allocate(std::span<char> memory){
        const auto capacity = memory.size() / Width;
        auto ptr = memory.data();
        data.position = {as<Vec>(ptr), capacity };
        ptr += capacity * sizeof(Vec);

        data.prePosition = {as<Vec>(ptr), capacity };
        ptr += capacity * sizeof(Vec);

        data.velocity = {as<Vec>(ptr), capacity};
        ptr += capacity * sizeof(Vec);

        data.inverseMass = {as<float>(ptr), capacity};
        ptr += capacity * sizeof(float);

        data.restitution = {as<float>(ptr), capacity};
        ptr += capacity * sizeof(float);

        data.radius = {as<float>(ptr), capacity};
    }

    static constexpr size_t allocationSize(size_t size) { return Width * size; }


    template<typename ValueType, Field field>
    class View{
    public:

        View(SeparateFieldMemoryLayout layout, size_t size = std::numeric_limits<size_t>::max())
                : m_layout{layout}
                , m_ptr(layout.get<ValueType, field>())
                , m_size{ size }
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
        size_t m_size{};

    };



    template<typename ValueType, Field field>
    [[nodiscard]]
    constexpr ValueType* get() const {
        switch(field){
            case Field::Position: return as<ValueType>(data.position.data());
            case Field::PreviousPosition: return as<ValueType>(data.prePosition.data());
            case Field::Velocity: return as<ValueType>(data.velocity.data());
            case Field::Mass: return as<ValueType>(data.inverseMass.data());
            case Field::Restitution: return as<ValueType>(data.restitution.data());
            case Field::Radius: return as<ValueType>(data.radius.data());
        }
        throw std::runtime_error{ "invalid field" };
    }

    auto position(size_t size) const {
        return View<VecType, Field::Position>{ *this, size };
    }

    auto previousPosition(size_t size) const {
        return View<VecType, Field::PreviousPosition>{ *this, size };

    }

    auto velocity(size_t size) const {
        return View<VecType, Field::Velocity>{ *this, size };
    }

    auto inverseMass(size_t size) const {
        return View<float, Field::Mass>{ *this, size };
    }

    auto restitution(size_t size) const {
        return View<float, Field::Restitution>{ *this, size };
    }

    auto radius(size_t size) const {
        return View<float, Field::Radius>{ *this, size };
    }

    void add(VecType pos, VecType prevPos, VecType vel, float invMass, float radius, float restitution, int index) {
        assert(index >= 0 && index < data.position.size());
        data.position[index] = pos;
        data.prePosition[index] = prevPos;
        data.velocity[index] = vel;
        data.inverseMass[index] = invMass;
        data.radius[index] = radius;
        data.restitution[index] = restitution;
    }

    [[nodiscard]]
    size_t capacity() const {
        return data.position.size();
    }

private:
    std::vector<char> memory;

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

using ProtoTypeParticle2D = InterleavedMemoryLayout2D::Members;

template<glm::length_t L>
inline InterleavedMemoryParticles<L> createInterleavedMemoryParticle(std::span<typename InterleavedMemoryParticles<L>::Layout::Members> span) {
    return { { span.data(), span.size() } };
}

inline InterleavedMemoryParticle2D createInterleavedMemoryParticle2D(std::span<typename InterleavedMemoryParticle2D::Layout::Members> span) {
    return { { span.data(), span.size() } };
}

inline std::shared_ptr<InterleavedMemoryParticle2D> createInterleavedMemoryParticle2DPtr(std::span<typename InterleavedMemoryParticle2D::Layout::Members> span) {
    return std::make_shared<InterleavedMemoryParticle2D>( InterleavedMemoryParticle2D{ span.data(), span.size() } );
}

inline SeparateFieldParticle2D createSeparateFieldParticle2D(std::span<glm::vec2> positions
        , std::span<glm::vec2> prevPosition
        , std::span<glm::vec2> velocity
        , std::span<float> inverseMass
        , std::span<float> restitution
        , std::span<float> radius) {
    return { { positions, prevPosition, velocity, inverseMass, restitution, radius }};
}

inline std::shared_ptr<SeparateFieldParticle2D> createSeparateFieldParticle2DPtr(std::span<glm::vec2> positions
        , std::span<glm::vec2> prevPosition
        , std::span<glm::vec2> velocity
        , std::span<float> inverseMass
        , std::span<float> restitution
        , std::span<float> radius) {
    return std::make_shared<SeparateFieldParticle2D>( SeparateFieldParticle2D{ {positions, prevPosition, velocity, inverseMass, restitution, radius} });
}

inline SeparateFieldParticle2D createSeparateFieldParticle2D(size_t numParticles){
    return { { numParticles } };
}

inline SeparateFieldParticle2D createSeparateFieldParticle2D(std::span<char> memory){
    return { { memory } };
}

inline std::shared_ptr<SeparateFieldParticle2D> createSeparateFieldParticle2DPtr(std::span<char> memory){
    auto particles = SeparateFieldParticle2D{ { memory } };
    return std::make_shared<SeparateFieldParticle2D>( particles );
}

template<template<typename> typename Layout>
inline YAML::Emitter& operator<<(YAML::Emitter& emitter, const Particle2D<Layout>& particles) {
    emitter << YAML::BeginMap;
    emitter << YAML::Key << "capacity" << YAML::Value << particles.size();
    emitter << YAML::Key << "fields" << YAML::Value;
    emitter << YAML::BeginSeq;
    for(int i = 0; i < particles.size(); i++){
        emitter << YAML::BeginMap;
        emitter << YAML::Key << "position";
        emitter << YAML::Value;
        emitter <<  YAML::BeginSeq;
        emitter << particles.position()[i].x;
        emitter << particles.position()[i].y;
        emitter << YAML::EndSeq;
        emitter << YAML::Key << "velocity";
        emitter << YAML::Value;
        emitter <<  YAML::BeginSeq;
        emitter << particles.velocity()[i].x;
        emitter << particles.velocity()[i].y;
        emitter << YAML::EndSeq;
        emitter << YAML::Key << "inverseMass";
        emitter << YAML::Value << particles.inverseMass()[i];
        emitter << YAML::Key << "restitution";
        emitter << YAML::Value << particles.restitution()[i];
        emitter << YAML::Key << "radius";
        emitter << YAML::Value << particles.radius()[i];
        emitter << YAML::EndMap;
    }
    emitter << YAML::EndSeq;
    emitter << YAML::EndMap;

    return emitter;
}

using Fields = InterleavedMemoryLayout2D::Members;

namespace YAML {
    template<>
    struct convert<Fields> {
        static Node encode(const Fields& rhs) {
            Node position{};
            position.push_back(rhs.position.x);
            position.push_back(rhs.position.y);

            Node velocity{};
            position.push_back(rhs.velocity.x);
            position.push_back(rhs.velocity.y);

            Node node{};
            node["position"] = position;
            node["velocity"] = velocity;
            node["inverseMass"] = rhs.inverseMass;
            node["restitution"] = rhs.restitution;
            node["radius"] = rhs.radius;
        }

        static bool decode(const Node& node, Fields& rhs){
            if(node.IsSequence()) {
                return false;
            }

            rhs.position.x = node["position"][0].as<float>();
            rhs.position.y = node["position"][1].as<float>();

            rhs.velocity.x = node["velocity"][0].as<float>();
            rhs.velocity.y = node["velocity"][1].as<float>();

            rhs.inverseMass = node["inverseMass"].as<float>();
            rhs.restitution = node["restitution"].as<float>();
            rhs.radius = node["radius"].as<float>();

            return true;
        }
    };
}

