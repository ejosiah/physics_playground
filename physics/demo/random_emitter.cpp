#include "random_emitter.h"
#include "random.h"

template<template<typename> typename Layout>
void RandomParticleEmitter<Layout>::RandomParticleEmitter::onUpdate(float currentTime, float deltaTime) {
    if(this->enabled()){
        this->disable();

        static auto seed = (1 << 20);

        auto pRand = random(m_bounds, seed);
        auto vRand = rng(0, 10, seed + (1 << 10));
        for(int i = 0; i < m_numParticles; i++){
            glm::vec2 position = pRand();
            glm::vec2 velocity{ vRand(), vRand() };
            this->m_particles->add(position, velocity, 1.0, m_radius, m_restitution);
        }
    }
}

template RandomParticleEmitter<InterleavedMemoryLayout>;
template RandomParticleEmitter<SeparateFieldMemoryLayout>;