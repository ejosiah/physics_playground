#pragma once

#include <glm/glm.hpp>
#include <entt/entt.hpp>
#include <vector>

#define VECTOR_COMPONENT(Typename)                  \
struct Typename{                                    \
    union {                                         \
        glm::vec2 value;                            \
        struct {                                    \
            float x; float y;                       \
        };                                          \
    };                                              \
                                                    \
    Typename& operator+=(glm::vec2 v) {             \
        this->value += v;                           \
        return *this;                               \
    }                                               \
                                                    \
    Typename& operator=(const glm::vec2& v) {       \
        this->value = v;                            \
        return *this;                               \
    }                                               \
                                                    \
    glm::vec2 operator*(float scale) const {        \
        return value * scale;                       \
    }                                               \
                                                    \
    operator glm::vec2() const {                    \
        return value;                               \
    }                                               \
};

struct Color {
    float r, g, b, a;
};

struct Particle{};

struct Outline{};
struct Filled{};
struct Hollow{};

struct PhysicsObject{};

struct Ref{  entt::entity id; };

struct Link{ entt::entity a; entt::entity b; };

struct Circle{ float radius{}; };
struct Line{ float length{}; float angle{}; };

VECTOR_COMPONENT(Position)
VECTOR_COMPONENT(Displacement)
VECTOR_COMPONENT(Velocity)
VECTOR_COMPONENT(Vector)

struct ContactNormal{ };

struct Box{ float width{}; float height{}; };
struct Instance{ int id{}; };
struct Layer{ int value{}; };

namespace vec_ops {

    template<typename T>
    inline auto magnitude(const T& t){
        return glm::length(t.value);
    }

    template<typename T>
    inline auto angle(const T& t){
        return glm::atan(t.y, t.x);
    }
}