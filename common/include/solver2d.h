#pragma once

#include "model2d.h"
#include <glm/glm.hpp>
#include <utility>
#include "sdf2d.h"

const float Gravity{-9.8};

enum class SolverType : uint32_t { Basic = 0 };

inline void boundsCheck(Particle2D& particle, const Bounds2D& bounds) {
    auto [min, max] = bounds;

    min += particle.radius;
    max -= particle.radius;
    auto& p = particle.cPosition;
    if(p.x < min.x){
        p.x = min.x;
        particle.velocity.x *= -1;
    }
    if(p.x > max.x){
        p.x = max.x;
        particle.velocity.x *= -1;
    }
    if(p.y < min.y){
        p.y = min.y;
        particle.velocity.y *= -1;
    }
    if(p.y > max.y){
        p.y = max.y;
        particle.velocity.y *= -1;
    }
}

inline glm::vec2 toSdfBox(const Bounds2D& bounds){
    auto [min, max] = bounds;
    return max - min;
}

template<typename ParticleLayout, SolverType solverType>
class Solver {
public:
    Solver() = default;

    explicit Solver(ParticleLayout&, Bounds2D, uint32_t numSubSteps = 1)
    {}

    void solve(float dt){};
};


template<>
class Solver<ArrayOfStructsLayout, SolverType::Basic> {
public:
    Solver() = default;

    explicit Solver(ArrayOfStructsLayout& particles, Bounds2D bounds, uint32_t numSubSteps = 1)
    : m_particles(particles)
    , m_numSubSteps{numSubSteps}
    , m_bounds{ toSdfBox(bounds) }
    {}

    void solve(float dt){
        const auto N = m_numSubSteps;
        float sdt = dt/static_cast<float>(N);
        for(int i = 0; i < N; ++i){
            subStep(sdt);
        }
    };

    void subStep(float dt){
        static auto const g = glm::vec2(0, Gravity);
        for(auto& p : m_particles){
            p.cPosition += p.velocity;
            p.velocity += g * dt;
//            boundsCheck(p, m_bounds);
        }
    }

protected:
    uint32_t m_numSubSteps{1};
    glm::vec2 m_bounds{};
    ArrayOfStructsLayout m_particles{};
};