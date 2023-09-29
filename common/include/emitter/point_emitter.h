#pragma once

#include "emitter.h"
#include <random>

template<glm::length_t L, typename Consumer>
class PointEmitter : public Emitter<L, Consumer> {
public:
    PointEmitter() = default;

    PointEmitter(
            const glm::vec<L, float>& origin
            , const glm::vec<L, float>& direction
            , float speed
            , float spreadAngleDeg
            , int maxNumOfNewParticlePerSecond = 1
            , int maxNumOfParticles = std::numeric_limits<int>::max()
            , uint32_t seed = 0)
            : m_origin{ origin }
            , m_direction{ direction }
            , m_speed{ speed }
            , m_spreadAngleRad{ glm::radians(spreadAngleDeg) }
            , maxNumberOfParticlePerSecond{ maxNumOfNewParticlePerSecond }
            , maxNumberOfParticles{ maxNumOfParticles }
            , m_rng{ seed }
    {}


    ~PointEmitter() override = default;

    void onUpdate(float currentTime, float deltaTime) override {
        auto& consumer = this->m_consumer;

        if(!consumer) return;

        auto elapsedTime = currentTime - m_firstFrameTimeInSeconds;
        auto newMaxTotalNumOfParticlesEmitted =
                to<int>(glm::ceil((elapsedTime + deltaTime) * this->maxNumberOfParticlePerSecond));

        newMaxTotalNumOfParticlesEmitted = glm::min(newMaxTotalNumOfParticlesEmitted, this->maxNumberOfParticles);

        auto maxNumberOfNewParticles = newMaxTotalNumOfParticlesEmitted - m_numberOfEmittedParticles;

        if(maxNumberOfNewParticles > 0) {
            emit(maxNumberOfNewParticles, deltaTime);
            m_numberOfEmittedParticles += maxNumberOfNewParticles;
        }

    }

    void emit(size_t maxNewNumberOfParticles, float deltaTime) {
        auto t = this->m_consumer.offset(deltaTime);
        for(size_t i = 0; i < maxNewNumberOfParticles; ++i){
            auto angle = (random() - 0.5) * m_spreadAngleRad;
            glm::mat2 rotate{ glm::cos(angle), glm::sin(angle), -glm::sin(angle), glm::cos(angle) };
            auto direction = rotate * m_direction;
            auto position = m_origin + direction * (i * t);
            this->m_consumer.use(position, m_speed * direction);
        }
    }

    void clear() final  {
        Emitter<L, Consumer>::clear();
        this->enable();
        m_numberOfEmittedParticles = 0;
        m_firstFrameTimeInSeconds = 0;
    }

private:
    float random() {
        static std::uniform_real_distribution<> dist(0.0, 1.0);
        return dist(m_rng);
    }

public:
    int maxNumberOfParticlePerSecond;
    int maxNumberOfParticles;

private:
    std::default_random_engine m_rng;
    float m_firstFrameTimeInSeconds{};
    int m_numberOfEmittedParticles{};

    glm::vec<L, float> m_origin;
    glm::vec<L, float> m_direction;
    float m_speed;
    float m_spreadAngleRad;

};