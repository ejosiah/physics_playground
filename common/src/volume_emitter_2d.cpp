#include "volume_emitter_2d.h"
#include "emitter/volume_emitter.h"

template<template<typename> typename Layout>
VolumeEmitter2D<Layout>::VolumeEmitter2D(std::function<float(const glm::vec2&)> sdf,
                                         std::unique_ptr<PointGenerator2D> pointGenerator,
                                         Bounds2D bounds,
                                         float spacing)
//: m_sdf(std::move(sdf))
//, m_pointGenerator(std::move(pointGenerator))
//, m_bounds(bounds)
//, m_spacing(spacing)
{
    ProtoTypeParticle2D prototype{};
    prototype.radius = spacing * .5;
    prototype.inverseMass = 1.f;
    prototype.restitution = 0.5;
    this->m_consumer.set(prototype);
    this->m_emitter = std::make_unique<VolumeEmitter<2, ParticlePointConsumer<Layout>>>(
            std::move(sdf),
            std::move(pointGenerator),
            bounds,
            spacing
    );
}

template VolumeEmitter2D<InterleavedMemoryLayout>;
template VolumeEmitter2D<SeparateFieldMemoryLayout>;