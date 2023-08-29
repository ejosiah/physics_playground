#pragma once
#include "model2d.h"
#include "particle.h"
#include "spacial_hash.h"
#include <glm/glm.hpp>
#include <memory>
#include <array>
#include <span>

template<template<typename> typename Layout>
inline void boundsCheck(Particle2D<Layout>& particle, const Bounds2D& bounds, int i) {
    auto [min, max] = bounds;

    auto radius = particle.radius()[i];
    auto& position = particle.position()[i];
    auto& velocity = particle.velocity()[i];

    min += radius;
    max -= radius;
    auto& p = position;
    if(p.x < min.x){
        p.x = min.x;
        velocity.x *= -1;
    }
    if(p.x > max.x){
        p.x = max.x;
        velocity.x *= -1;
    }
    if(p.y < min.y){
        p.y = min.y;
        velocity.y *= -1;
    }
    if(p.y > max.y){
        p.y = max.y;
        velocity.y *= -1;
    }
}

template<template<typename> typename Layout>
class CollisionHandler {
public:
    struct CollisionStats{
        std::array<int, 100> average{};
        int max{0};
        int min{0};
        int next{0};
    };

    CollisionStats collisionStats{};

    CollisionHandler() = default;

    CollisionHandler(std::shared_ptr<Particle2D<Layout>> particles
            , Bounds2D worldBounds
            , float maxRadius);

    void resolveCollision();

    int resolveCollision(int ia, int ib);

private:
    Bounds2D m_worldBounds;
    std::shared_ptr<Particle2D<Layout>> m_particles;
    Particle2D<Layout>::Position m_position;
    Particle2D<Layout>::PreviousPosition m_prevPosition;
    Particle2D<Layout>::Velocity m_velocity;
    Particle2D<Layout>::InverseMass m_inverseMass;
    Particle2D<Layout>::Restitution m_restitution;
    Particle2D<Layout>::Radius m_radius;
    SpacialHashGrid2D<> grid;
    float m_maxRadius{};
};

template<template<typename> typename Layout>
CollisionHandler<Layout>::CollisionHandler(std::shared_ptr<Particle2D<Layout>>  particles
        , Bounds2D worldBounds
        , float maxRadius)
        : m_particles{ particles }
        , m_position{ particles->position() }
        , m_prevPosition{ particles->previousPosition() }
        , m_velocity{ particles->velocity() }
        , m_inverseMass{ particles->inverseMass() }
        , m_restitution{ particles->restitution() }
        , m_radius{ particles->radius() }
        , m_worldBounds{ worldBounds }
        , m_maxRadius{ maxRadius }
{
    grid = SpacialHashGrid2D{m_maxRadius * 2, to<int32_t>(particles->capacity()) };
}

template<template<typename> typename Layout>
void CollisionHandler<Layout>::resolveCollision() {
    const auto numParticles = m_particles->size();
    grid.initialize(*m_particles, numParticles);

    auto gridSpacing = m_maxRadius * 2;
//    auto bounds = shrink(m_worldBounds, gridSpacing);
    auto bounds = m_worldBounds;
    auto [sizeX, sizeY] = dimensions(bounds);
    auto numX = to<int>(glm::floor(sizeX/gridSpacing));
    auto numY = to<int>(glm::floor(sizeY/gridSpacing));


    auto& vPositions = m_position;
//    for(int i = 0; i < numParticles; i++){
//        auto& position = vPositions[i];
//        auto ids = grid.query(position, glm::vec2(gridSpacing));
//
//        int collisions = 0;
//        for(int j : ids){
//            if(i == j) continue;
//            collisions += resolveCollision(i, j);
//        }
//        boundsCheck(*m_particles, m_worldBounds, i);
//
//        collisionStats.average[collisionStats.next++] = collisions;
//        collisionStats.max = glm::max(collisionStats.max, collisions);
//        collisionStats.min = glm::min(collisionStats.min, collisions);
//
//        collisionStats.next %= collisionStats.average.size();
//    }

    for(auto y = 1; y < numY - 1; y++){
        for(auto x = 1; x < numX - 1; x++){
            auto position = glm::vec2{x, y} * gridSpacing + m_maxRadius;
            auto ids = grid.query(position, glm::vec2(gridSpacing));

            int collisions = 0;
            const auto N = ids.size();
            for(auto i : ids){
                for(auto j : ids) {
                    if (i == j) continue;
                    collisions += resolveCollision(i, j);
                }
            }

            if(collisions != 0) {
                collisionStats.average[collisionStats.next++] = collisions;
                collisionStats.max = glm::max(collisionStats.max, collisions);
                collisionStats.min = glm::min(collisionStats.min, collisions);

                collisionStats.next %= collisionStats.average.size();
            }
        }
    }
    for(auto i = 0; i < numParticles; i++){
        boundsCheck(*m_particles, m_worldBounds, i);
    }
}

template<template<typename> typename Layout>
int CollisionHandler<Layout>::resolveCollision(int ia, int ib) {
    auto& position = m_position;
    auto& velocity = m_velocity;
    auto& inverseMass = m_inverseMass;
    auto& restitution = m_restitution;
    auto& radius = m_radius;

    auto& pa = position[ia];
    auto& pb = position[ib];
    glm::vec2 dir = pb - pa;
    auto rr = radius[ia] + radius[ib];
    rr *= rr;
    auto dd = glm::dot(dir, dir);
    if(dd == 0 || dd > rr) return 0;

    auto d = glm::sqrt(dd);
    dir /= d;

    auto corr = (radius[ib] + radius[ia] - d) * .5f;
    pa -= dir * corr;
    pb += dir * corr;

    auto v1 = glm::dot(velocity[ia], dir);
    auto v2 = glm::dot(velocity[ib], dir);

    auto m1 = 1/inverseMass[ia];
    auto m2 = 1/inverseMass[ib];

    auto newV1 = (m1 * v1 + m2 * v2 - m2 * (v1 - v2) * restitution[ia]) / (m1 + m2);
    auto newV2 = (m1 * v1 + m2 * v2 - m1 * (v2 - v1) * restitution[ib]) / (m1 + m2);

    velocity[ia] += dir * (newV1 - v1);
    velocity[ib] += dir * (newV2 - v2);
    return 1;
}