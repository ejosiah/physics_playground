#pragma once

#include "emitter.h"
#include "point_generators.h"

template<glm::length_t L, typename Consumer>
class VolumeEmitter : public Emitter<L, Consumer> {
public:
    VolumeEmitter() = default;

    VolumeEmitter(
            std::function<float(const glm::vec<L, float>&)> sdf,
            std::unique_ptr<PointGenerator<L>> pointGenerator,
            Bounds<L> bounds,
            float spacing)
            : m_sdf(std::move(sdf))
            , m_pointGenerator(std::move(pointGenerator))
            , m_bounds(bounds)
            , m_spacing(spacing)
    {
    }

    ~VolumeEmitter() override = default;

    void onUpdate(float currentTime, float deltaTime) override {
        auto consumer = this->m_consumer;
        if(!consumer){
            return;
        }

        auto sdf = m_sdf;
        auto points = m_pointGenerator->generate(m_bounds, m_spacing);
        for(const auto& point : points){
            if(sdf(point) < 0){
                consumer.use(point);
            }
        }
        this->disable();
    }

    void set(std::unique_ptr<PointGenerator<L>> pointGenerator) {
        m_pointGenerator = std::move(pointGenerator);
    }

private:
private:
    std::function<float(const glm::vec2&)> m_sdf;
    std::unique_ptr<PointGenerator2D> m_pointGenerator{};
    Bounds2D m_bounds{};
    float m_spacing{0};

};