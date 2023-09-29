#pragma once

#include "particle_emitter.h"
#include "point_generators.h"
#include <glm/glm.hpp>
#include <random>
#include <limits>
#include <functional>
#include <memory>

template<template<typename> typename Layout>
struct VolumeEmitter2D : public ParticleEmitter<Layout> {
public:
    VolumeEmitter2D(
            std::function<float(const glm::vec2&)> sdf,
            std::unique_ptr<PointGenerator2D> pointGenerator,
            Bounds2D bounds,
            float spacing);

    VolumeEmitter2D() = default;

    ~VolumeEmitter2D() override = default;

    void onUpdate(float currentTime, float deltaTime) final {}


};


