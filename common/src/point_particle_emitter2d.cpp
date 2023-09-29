#include "point_particle_emitter2d.h"
#include <spdlog/spdlog.h>

template<template<typename> typename Layout>
PointParticleEmitter2D<Layout>::PointParticleEmitter2D(const glm::vec2& origin
                                                       , const glm::vec2& direction
                                                       , float speed
                                                       , float spreadAngleDeg
                                                       , int maxNumOfNewParticlePerSecond
                                                       , int maxNumOfParticles
                                                       , uint32_t seed
                                                       , ProtoTypeParticle2D prototype)
                                                       {
    this->m_emitter = std::make_unique<PointEmitter<2, ParticlePointConsumer<Layout>>>(
            origin, direction, speed, spreadAngleDeg, maxNumOfNewParticlePerSecond, maxNumOfParticles, seed
            );
    this->m_consumer.set(prototype);
}

template<template<typename> typename Layout>
PointParticleEmitter2D<Layout>::Builder PointParticleEmitter2D<Layout>::builder() {
    return Builder();
}


template PointParticleEmitter2D<InterleavedMemoryLayout>;
template PointParticleEmitter2D<SeparateFieldMemoryLayout>;