#pragma once

#include "particle_emitter.h"
#include "point_generator2d.h"
#include <glm/glm.hpp>
#include <random>
#include <limits>
#include <functional>
#include <memory>

template<template<typename> typename Layout>
class VolumeEmitter2D : public ParticleEmitter<Layout> {
public:
    VolumeEmitter2D(
            std::function<float(const glm::vec2&)> sdf,
            std::unique_ptr<PointGenerator2D> pointGenerator,
            Bounds2D bounds,
            float spacing);

    VolumeEmitter2D() = default;

    ~VolumeEmitter2D() override = default;

    void onUpdate(float currentTime, float deltaTime) override;

    void set(std::unique_ptr<PointGenerator2D> pointGenerator) {
        m_pointGenerator = std::move(pointGenerator);
    }

private:
    std::function<float(const glm::vec2&)> m_sdf;
    std::unique_ptr<PointGenerator2D> m_pointGenerator{};
    Bounds2D m_bounds{};
    float m_spacing{0};
};



