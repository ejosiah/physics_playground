#include <benchmark/benchmark.h>
#include <random>
#include <algorithm>
#include <glm/glm.hpp>

struct Particle {
    glm::vec2 position;
    glm::vec2 velocity;
};

struct Particles {
    std::vector<glm::vec2> positions;
    std::vector<glm::vec2> velocity;
};

static constexpr glm::vec2 GRAVITY{0, -9.8};

class MemoryAccessFixture : public benchmark::Fixture {
protected:
    std::default_random_engine engine{ (1 << 20) };
    std::uniform_real_distribution<float> pos_dist{0, 20};
    std::uniform_real_distribution<float> vel_dist{-5, 5};
    static constexpr float dt = 0.01666667;
};

BENCHMARK_DEFINE_F(MemoryAccessFixture, independentUpdatesArrayOfStructures)(benchmark::State& state){
    const auto N = state.range(0);
    std::vector<Particle> particles(N);

    std::generate(particles.begin(), particles.end(), [this]{
        Particle particle{};
        particle.position.x = pos_dist(engine);
        particle.position.y = pos_dist(engine);
        particle.velocity.x = vel_dist(engine);
        particle.velocity.y = vel_dist(engine);
        return particle;
    });

    for(auto _ : state){
        for(auto& particle : particles){
            particle.velocity += GRAVITY * dt;
        }
        for(auto& particle : particles){
            particle.position += particle.velocity * dt;
        }
    }
}

BENCHMARK_DEFINE_F(MemoryAccessFixture, groupUpdatesArrayOfStructures)(benchmark::State& state){
    const auto N = state.range(0);
    std::vector<Particle> particles(N);

    std::generate(particles.begin(), particles.end(), [this]{
        Particle particle{};
        particle.position.x = pos_dist(engine);
        particle.position.y = pos_dist(engine);
        particle.velocity.x = vel_dist(engine);
        particle.velocity.y = vel_dist(engine);
        return particle;
    });

    for(auto _ : state){
        for(auto& particle : particles){
            particle.velocity += GRAVITY * dt;
            particle.position += particle.velocity * dt;

        }
    }
}

BENCHMARK_REGISTER_F(MemoryAccessFixture, independentUpdatesArrayOfStructures)->RangeMultiplier(2)->Range(32, 32<<15);
BENCHMARK_REGISTER_F(MemoryAccessFixture, groupUpdatesArrayOfStructures)->RangeMultiplier(2)->Range(32, 32<<15);