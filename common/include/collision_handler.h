#pragma once
#include "model2d.h"
#include "particle.h"
#include "spacial_hash.h"
#include "c_debug.h"
#include <glm/glm.hpp>
#include <memory>
#include <array>
#include <span>
#include <thread>
#include <barrier>
#include <latch>
#include <spdlog/spdlog.h>
#include <glm/gtc/epsilon.hpp>

template<template<typename> typename Layout>
inline void boundsCheck(Particle2D<Layout>& particle, const Bounds2D& bounds, int i) {
    auto radius = particle.radius()[i];
    auto& position = particle.position()[i];
    auto& velocity = particle.velocity()[i];
    bool collides = false;

    auto [min, max] = shrink(bounds, radius);

    auto& p = position;
    if(p.x < min.x){
        p.x = min.x;
        velocity.x *= -1;
        collides = true;
    }
    if(p.x > max.x){
        p.x = max.x;
        velocity.x *= -1;
        collides = true;
    }
    if(p.y < min.y){
        p.y = min.y;
        velocity.y *= -1;
        // remove velocity caused by gravity
        // FIXME remove magic number and pass in gravity and frame rate
        velocity.y -= 0.163333f;
        velocity.y = glm::max(0.f, velocity.y);
        collides = true;
    }
    if(p.y > max.y){
        p.y = max.y;
        velocity.y *= -1;
        collides = true;
    }
    if(collides){
        boundCollisions.push_back(i);
    }
}

inline bool contains(const Bounds2D& bounds, const glm::vec2& p){
    auto [min, max] = bounds;
    return
        p.x >= min.x && p.x < max.x &&
        p.y >= min.y && p.y < max.y;
}

struct Contact {
    int ia{};
    int ib{};
    float d{};
    glm::vec2 normal{};
};

template<template<typename> typename Layout>
class CollisionHandler {
public:
    struct CollisionStats{
        std::array<int, 100> average{};
        int max{0};
        int min{0};
        int next{0};
        int total{0};
    };

    CollisionStats collisionStats{};

    CollisionHandler() = default;

    CollisionHandler(std::shared_ptr<Particle2D<Layout>> particles
            , Bounds2D worldBounds
            , float maxRadius, bool verlet = false);

    void resolveCollision();

    void aBoundsCheck();

    int generateContact(int ia, int ib);

    int resolveCollision(int ia, int ib);

    std::thread worker(int id, int numWorkers);

private:
    Bounds2D m_worldBounds;
    std::shared_ptr<Particle2D<Layout>> m_particles;
    BoundedSpacialHashGrid2D grid;
    std::vector<Contact> m_contacts;
    size_t m_numContacts{};
    float m_maxRadius{};
    int numThreads{1};
    std::barrier<> m_syncPoint{0};
    std::latch m_startLatch{0};
    std::vector<std::thread> workers;
    bool m_verlet{false};
};

template<template<typename> typename Layout>
std::thread CollisionHandler<Layout>::worker(int id, int numWorkers) {
    return std::thread([id, numWorkers, this]{
        m_startLatch.wait();
        auto gridSpacing = m_maxRadius * 2;

        auto gridSize = m_worldBounds.upper - m_worldBounds.lower;
        auto [sizeX, sizeY] = dimensions(m_worldBounds);
        auto ratio = 1.f/to<float>(numWorkers);
        Bounds2D bounds = m_worldBounds;
        bounds.upper.x *= ratio;
        bounds.upper.x += to<float>(id) * sizeX * ratio;
        bounds.lower.x += to<float>(id) * sizeX * ratio;

        spdlog::info("worker({}) working on bounds({}, {})", id, bounds.lower, bounds.upper);
    });
}

template<template<typename> typename Layout>
CollisionHandler<Layout>::CollisionHandler(std::shared_ptr<Particle2D<Layout>>  particles
        , Bounds2D worldBounds
        , float maxRadius
        , bool verlet)
        : m_particles{ particles }
        , m_worldBounds{ worldBounds }
        , m_maxRadius{ maxRadius }
        , m_syncPoint{ numThreads + 1 }
        , m_startLatch{ numThreads }
        ,  m_contacts( particles->capacity())
        , m_verlet(verlet)
{
//    grid = SpacialHashGrid2D{m_maxRadius * 2, to<int32_t>(particles->capacity()) };
    glm::ivec2 gridSize = worldBounds.upper - worldBounds.lower;
    grid = BoundedSpacialHashGrid2D{m_maxRadius * 2, gridSize };
}

template<template<typename> typename Layout>
void CollisionHandler<Layout>::resolveCollision() {
    const auto numParticles = m_particles->size();
    grid.initialize(*m_particles, numParticles);

    auto vPositions = m_particles->position();

    for(int i = 0; i < numParticles; i++){
        auto& position = vPositions[i];

        auto ids = grid.query(position, glm::vec2(m_maxRadius * 2));

        int collisions = 0;
        for(int j : ids){
            if(i == j) continue;
            collisions += resolveCollision(i, j);
        }
        collisionStats.average[collisionStats.next++] = collisions;
        collisionStats.max = glm::max(collisionStats.max, collisions);
        collisionStats.min = glm::min(collisionStats.min, collisions);

        collisionStats.next %= collisionStats.average.size();
        collisionStats.total += collisions;
    }

    for(auto i = 0; i < numParticles; i++){
        boundsCheck(*m_particles, m_worldBounds, i);
    }
}

template<template<typename> typename Layout>
void CollisionHandler<Layout>::aBoundsCheck() {
    const auto numParticles = m_particles->size();
    for(auto i = 0; i < numParticles; i++){
        boundsCheck(*m_particles, m_worldBounds, i);
    }
}

template<template<typename> typename Layout>
int CollisionHandler<Layout>::generateContact(int ia, int ib) {
    auto position = m_particles->position();
    auto radius = m_particles->radius();

    auto& pa = position[ia];
    auto& pb = position[ib];
    glm::vec2 dir = pb - pa;
    auto rr = radius[ia] + radius[ib];
    rr *= rr;
    auto dd = glm::dot(dir, dir);
    if(dd == 0 || dd >= rr) return 0;

    auto d = glm::sqrt(dd);
    m_contacts[m_numContacts].ia = ia;
    m_contacts[m_numContacts].ib = ib;
    m_contacts[m_numContacts].d = d;
    m_contacts[m_numContacts].normal = dir/d;
    m_numContacts++;
    return 1;
}


template<template<typename> typename Layout>
int CollisionHandler<Layout>::resolveCollision(int ia, int ib) {
    auto position = m_particles->position();
    auto prevPosition = m_particles->previousPosition();
    auto velocity = m_particles->velocity();
    auto inverseMass = m_particles->inverseMass();
    auto restitution = m_particles->restitution();
    auto radius = m_particles->radius();

    auto& pa = position[ia];
    auto& pb = position[ib];
    glm::vec2 dir = pb - pa;
    auto n = dir;
    auto rr = radius[ia] + radius[ib];
    rr *= rr;
    auto dd = glm::dot(dir, dir);
    if(dd == 0 || dd > rr) return 0;

    auto d = glm::sqrt(dd);
    dir /= d;

    auto rest = (restitution[ia] + restitution[ib]) * 0.5f;
    if(m_verlet){
        auto corr = rest * (radius[ib] + radius[ia] - d) * .5f;
        auto vel = (n / d) * corr;
        pa -= vel;
        pb += vel;
    }else {
        auto corr = (radius[ib] + radius[ia] - d) * .5f;
        pa -= dir * corr;
        pb += dir * corr;

        auto v1 = glm::dot(velocity[ia], dir);
        auto v2 = glm::dot(velocity[ib], dir);
        auto vs = dot(velocity[ib] - velocity[ia], dir);

        auto m1 = 1 / inverseMass[ia];
        auto m2 = 1 / inverseMass[ib];

        auto cvs = -vs * rest;
        auto newV1 = (m1 * v1 + m2 * v2 - m2 * -cvs) / (m1 + m2);
        auto newV2 = (m1 * v1 + m2 * v2 - m1 * cvs) / (m1 + m2);

        velocity[ia] += dir * (newV1 - v1);
        velocity[ib] += dir * (newV2 - v2);
    }
    boundsCheck(*m_particles, m_worldBounds, ia);
    boundsCheck(*m_particles, m_worldBounds, ib);

    ballCollisions.push_back(ia);
    ballCollisions.push_back(ib);
    return 1;
}