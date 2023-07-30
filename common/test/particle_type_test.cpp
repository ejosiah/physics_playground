#include "particle_type_fixture.h"
#include <type_traits>

TEST_F(ParticleTypeFixture, InterleavedMemoryLayoutPositionView) {
    std::vector<InterleavedMemoryLayout<glm::vec2>::Members> source{
            {glm::vec2{0}}, {glm::vec2{1}}, {glm::vec3{3}}
    };

    auto particles = createInterleavedMemoryParticle2D(source);
    auto view = particles.position();

    ASSERT_EQ(view[0].x, 0);
    ASSERT_EQ(view[0].x, 0);

    ASSERT_EQ(view[1].x, 1);
    ASSERT_EQ(view[1].x, 1);

    ASSERT_EQ(view[2].x, 3);
    ASSERT_EQ(view[2].x, 3);

    view[2] = glm::vec2{2, 8};
    ASSERT_EQ(view[2].x, 2);
    ASSERT_EQ(view[2].y, 8);
}

TEST_F(ParticleTypeFixture, InterleavedMemoryLayoutConstPositionView) {
    std::vector<InterleavedMemoryLayout<glm::vec2>::Members> source{
            {glm::vec2{0}}, {glm::vec2{1}}, {glm::vec3{3}}
    };

    auto particles = createInterleavedMemoryParticle2D(source);
    const auto view = particles.position();

    ASSERT_EQ(view[0].x, 0);
    ASSERT_EQ(view[0].x, 0);

    ASSERT_EQ(view[1].x, 1);
    ASSERT_EQ(view[1].x, 1);

    ASSERT_EQ(view[2].x, 3);
    ASSERT_EQ(view[2].x, 3);
}

TEST_F(ParticleTypeFixture, InterleavedMemoryLayoutVelocityView) {
    std::vector<InterleavedMemoryLayout<glm::vec2>::Members> source{
            {glm::vec2{0}, glm::vec2(0), glm::vec3(0.5)},
            {glm::vec2{1}, glm::vec2(0), glm::vec2(1.2, 2.6)},
            {glm::vec3{3}, glm::vec2(0), glm::vec2(0.7, 0.1)}
    };

    auto particles = createInterleavedMemoryParticle2D(source);
    auto view = particles.velocity();

    ASSERT_FLOAT_EQ(view[0].x, 0.5);
    ASSERT_FLOAT_EQ(view[0].y, 0.5);

    ASSERT_FLOAT_EQ(view[1].x, 1.2);
    ASSERT_FLOAT_EQ(view[1].y, 2.6);

    ASSERT_FLOAT_EQ(view[2].x, 0.7);
    ASSERT_FLOAT_EQ(view[2].y, 0.1);
}

TEST_F(ParticleTypeFixture, InterleavedMemoryLayoutMassView) {
    std::vector<InterleavedMemoryLayout<glm::vec2>::Members> source{
            {glm::vec2{0}, glm::vec2(0), glm::vec3(0.5), 0.1},
            {glm::vec2{1}, glm::vec2(0), glm::vec2(1.2, 2.6), 2.5},
            {glm::vec3{3}, glm::vec2(0), glm::vec2(0.7, 0.1), 0.01}
    };

    auto particles = createInterleavedMemoryParticle2D(source);
    auto view = particles.inverseMass();

    ASSERT_FLOAT_EQ(view[0], 0.1);
    ASSERT_FLOAT_EQ(view[1], 2.5);
    ASSERT_FLOAT_EQ(view[2], 0.01);
}

TEST_F(ParticleTypeFixture, InterleavedMemoryLayoutRestitutionView) {
    std::vector<InterleavedMemoryLayout<glm::vec2>::Members> source{
            {glm::vec2{0}, glm::vec2(0), glm::vec3(0.5), 0.1, 0.8},
            {glm::vec2{1}, glm::vec2(0), glm::vec2(1.2, 2.6), 2.5, 0.99},
            {glm::vec3{3}, glm::vec2(0), glm::vec2(0.7, 0.1), 0.01, 0.5}
    };

    auto particles = createInterleavedMemoryParticle2D(source);
    auto view = particles.restitution();

    ASSERT_FLOAT_EQ(view[0], 0.8);
    ASSERT_FLOAT_EQ(view[1], 0.99);
    ASSERT_FLOAT_EQ(view[2], 0.5);
}

TEST_F(ParticleTypeFixture, InterleavedMemoryLayoutRadiusView) {
    std::vector<InterleavedMemoryLayout<glm::vec2>::Members> source{
            {glm::vec2{0}, glm::vec2(0), glm::vec3(0.5), 0.1, 0.8, 1.0},
            {glm::vec2{1}, glm::vec2(0), glm::vec2(1.2, 2.6), 2.5, 0.99, 2.5},
            {glm::vec3{3}, glm::vec2(0), glm::vec2(0.7, 0.1), 0.01, 0.5, 0.123}
    };

    auto particles = createInterleavedMemoryParticle2D(source);
    auto view = particles.radius();

    ASSERT_FLOAT_EQ(view[0], 1.0);
    ASSERT_FLOAT_EQ(view[1], 2.5);
    ASSERT_FLOAT_EQ(view[2], 0.123);
}

TEST_F(ParticleTypeFixture, InterleavedMemoryLayoutViewIterator) {
    std::vector<InterleavedMemoryLayout<glm::vec2>::Members> source{
            {glm::vec2{0}}, {glm::vec2{1}}, {glm::vec3{3}}
    };

    auto particles = createInterleavedMemoryParticle2D(source);
    auto view = particles.position();

    int i = 0;
    for(auto position : view){
        ASSERT_FLOAT_EQ(source[i].position.x, position.x);
        ASSERT_FLOAT_EQ(source[i].position.y, position.y);
        i++;
    }
}