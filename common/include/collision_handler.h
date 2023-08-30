#pragma once
#include "model2d.h"
#include "particle.h"
#include "spacial_hash.h"
#include <glm/glm.hpp>
#include <memory>
#include <array>
#include <span>
#include <thread>
#include <barrier>
#include <latch>
#include <spdlog/spdlog.h>

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
    };

    CollisionStats collisionStats{};

    CollisionHandler() = default;

    CollisionHandler(std::shared_ptr<Particle2D<Layout>> particles
            , Bounds2D worldBounds
            , float maxRadius);

    void resolveCollision();

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
    std::barrier<> m_syncPoint;
    std::latch m_startLatch;
    std::vector<std::thread> workers;
};

template<template<typename> typename Layout>
std::thread CollisionHandler<Layout>::worker(int id, int numWorkers) {
    return std::thread([id, numWorkers, this]{
        m_startLatch.wait();
        auto gridSpacing = m_maxRadius * 2;

        while(true){
            m_syncPoint.arrive_and_wait();
            const auto numParticles = m_particles->size();

            auto vPositions = m_particles->position();
            for(int i = 0; i < numParticles; i++){
                auto& position = vPositions[i];
                auto ids = grid.query(position, glm::vec2(gridSpacing));

                int collisions = 0;
                for(int j : ids){
                    if(i == j) continue;
                    collisions += resolveCollision(i, j);
                }
                collisionStats.average[collisionStats.next++] = collisions;
                collisionStats.max = glm::max(collisionStats.max, collisions);
                collisionStats.min = glm::min(collisionStats.min, collisions);

                collisionStats.next %= collisionStats.average.size();
            }

            for(auto i = 0; i < numParticles; i++){
                boundsCheck(*m_particles, m_worldBounds, i);
            }
            m_syncPoint.arrive_and_wait();
        }
    });
}

template<template<typename> typename Layout>
CollisionHandler<Layout>::CollisionHandler(std::shared_ptr<Particle2D<Layout>>  particles
        , Bounds2D worldBounds
        , float maxRadius)
        : m_particles{ particles }
        , m_worldBounds{ worldBounds }
        , m_maxRadius{ maxRadius }
        , m_syncPoint{ numThreads + 1 }
        , m_startLatch{ numThreads }
        ,  m_contacts( particles->capacity())
{
//    grid = SpacialHashGrid2D{m_maxRadius * 2, to<int32_t>(particles->capacity()) };
    glm::ivec2 gridSize = worldBounds.upper - worldBounds.lower;
    grid = BoundedSpacialHashGrid2D{m_maxRadius * 2, gridSize };
}

template<template<typename> typename Layout>
void CollisionHandler<Layout>::resolveCollision() {
    static bool firstCall = true;
    if(firstCall){
        for(auto i = 0; i < numThreads; i++){
            auto thread = worker(i, numThreads);
            workers.push_back(std::move(thread));
        }
        m_startLatch.count_down(numThreads);
        firstCall = false;
    }

    const auto numParticles = m_particles->size();
    grid.initialize(*m_particles, numParticles);
    m_syncPoint.arrive_and_wait();
    m_syncPoint.arrive_and_wait();

//    const auto numParticles = m_particles->size();
//    grid.initialize(*m_particles, numParticles);
//
//    auto gridSpacing = m_maxRadius * 2;
//    auto vPositions = m_particles->position();
//    for(int i = 0; i < numParticles; i++){
//        auto& position = vPositions[i];
//        auto ids = grid.query(position, glm::vec2(gridSpacing));
//
//        int collisions = 0;
//        for(int j : ids){
//            if(i == j) continue;
//            collisions += resolveCollision(i, j);
//        }
//        collisionStats.average[collisionStats.next++] = collisions;
//        collisionStats.max = glm::max(collisionStats.max, collisions);
//        collisionStats.min = glm::min(collisionStats.min, collisions);
//
//        collisionStats.next %= collisionStats.average.size();
//    }
//
//   for(auto i = 0; i < numParticles; i++){
//        boundsCheck(*m_particles, m_worldBounds, i);
//    }
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
    if(dd == 0 || dd > rr) return 0;

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
    auto velocity = m_particles->velocity();
    auto inverseMass = m_particles->inverseMass();
    auto restitution = m_particles->restitution();
    auto radius = m_particles->radius();

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

    auto rest = (restitution[ia] + restitution[ib]) * 0.5f;

    auto newV1 = (m1 * v1 + m2 * v2 - m2 * (v1 - v2) * rest) / (m1 + m2);
    auto newV2 = (m1 * v1 + m2 * v2 - m1 * (v2 - v1) * rest) / (m1 + m2);

    velocity[ia] += dir * (newV1 - v1);
    velocity[ib] += dir * (newV2 - v2);
    boundsCheck(*m_particles, m_worldBounds, ia);
    boundsCheck(*m_particles, m_worldBounds, ib);
    return 1;
}