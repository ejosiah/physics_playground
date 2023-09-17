#include "volume_emitter_2d.h"

template<template<typename> typename Layout>
VolumeEmitter2D<Layout>::VolumeEmitter2D(std::function<float(const glm::vec2&)> sdf,
                                         std::unique_ptr<PointGenerator2D> pointGenerator,
                                         Bounds2D bounds,
                                         float spacing)
: m_sdf(std::move(sdf))
, m_pointGenerator(std::move(pointGenerator))
, m_bounds(bounds)
, m_spacing(spacing)
{
}

template<template<typename> typename Layout>
void VolumeEmitter2D<Layout>::onUpdate(float currentTime, float deltaTime) {
    auto particles = this->target();

    if(!particles || !this->enabled()){
        return;
    }

    auto sdf = m_sdf;
    auto points = m_pointGenerator->generate(m_bounds, m_spacing);
    for(const auto& point : points){
        if(sdf(point) < 0){
            particles->add(point, glm::vec2(0), 1, m_spacing * .5f, 0.5);
        }
    }
    this->disable();
}

template VolumeEmitter2D<InterleavedMemoryLayout>;
template VolumeEmitter2D<SeparateFieldMemoryLayout>;