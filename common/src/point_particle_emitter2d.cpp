#include "point_particle_emitter2d.h"
#include <spdlog/spdlog.h>

template<template<typename> typename Layout>
PointParticleEmitter2D<Layout>::PointParticleEmitter2D(const glm::vec2 origin
                                                       , const glm::vec2 direction
                                                       , float speed
                                                       , float spreadAngleDeg
                                                       , size_t maxNumOfNewParticlePerSecond
                                                       , size_t maxNumOfParticles
                                                       , uint32_t seed
                                                       , ProtoTypeParticle2D prototype)
: m_origin{ origin }
, m_direction{ direction }
, m_speed{ speed }
, m_spreadAngleRad{ glm::radians(spreadAngleDeg) }
, maxNumberOfParticlePerSecond{ maxNumOfNewParticlePerSecond }
, maxNumberOfParticles{ maxNumOfParticles }
, m_rng{ seed }
, m_prototype{ prototype }
{}

template<template<typename> typename Layout>
void PointParticleEmitter2D<Layout>::onUpdate(float currentTime, float deltaTime) {
    auto particles = this->target();

    if(!particles){
        return;
    }

    if(m_numberOfEmittedParticles == 0){
        m_firstFrameTimeInSeconds = currentTime;
    }

    auto elapsedTime = currentTime - m_firstFrameTimeInSeconds;
    auto newMaxTotalNumOfParticlesEmitted =
            to<size_t>(glm::ceil((elapsedTime + deltaTime) * this->maxNumberOfParticlePerSecond));

    newMaxTotalNumOfParticlesEmitted = glm::min(newMaxTotalNumOfParticlesEmitted, this->maxNumberOfParticles);

    auto maxNumberOfNewParticles = newMaxTotalNumOfParticlesEmitted - m_numberOfEmittedParticles;

    if(maxNumberOfNewParticles > 0) {
        emit(maxNumberOfNewParticles);

        m_numberOfEmittedParticles += particles->size();
    }

    spdlog::info("{}: totalToEmit {}, emitted: {}", elapsedTime, newMaxTotalNumOfParticlesEmitted, m_numberOfEmittedParticles);

}

template<template<typename> typename Layout>
void PointParticleEmitter2D<Layout>::emit(size_t maxNewNumberOfParticles) {
    auto particles = this->target();
    for(size_t i = 0; i < maxNewNumberOfParticles; ++i){
        auto angle = (random() - 0.5) * m_spreadAngleRad;
        glm::mat2 rotate{ glm::cos(angle), glm::sin(angle), -glm::sin(angle), glm::cos(angle) };
        this->add(m_origin, m_speed * (rotate * m_direction));
    }
}

template<template<typename> typename Layout>
float PointParticleEmitter2D<Layout>::random() {
    static std::uniform_real_distribution<> dist(0.0, 1.0);
    return dist(m_rng);
}

template<template<typename> typename Layout>
void PointParticleEmitter2D<Layout>::add(glm::vec2 position, glm::vec2 velocity) {
    auto particles = this->target();

    particles->add(position, velocity, m_prototype.inverseMass, m_prototype.radius, m_prototype.restitution);
}

template<template<typename> typename Layout>
PointParticleEmitter2D<Layout>::Builder PointParticleEmitter2D<Layout>::builder() {
    return Builder();
}


template PointParticleEmitter2D<InterleavedMemoryLayout>;
template PointParticleEmitter2D<SeparateFieldMemoryLayout>;