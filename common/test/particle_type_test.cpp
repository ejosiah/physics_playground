#include "particle_type_fixture.h"
#include <type_traits>

TEST_F(ParticleTypeFixture, InterleavedMemoryLayoutPositionView) {
    std::vector<InterleavedMemoryLayout<glm::vec2>::Members> source{
            {glm::vec2{0}}, {glm::vec2{1}}, {glm::vec3{3}}
    };

//    InterleavedMemoryLayout<glm::vec2> layout{ source.data(), source.size() };
//    InterleavedMemoryParticle2D particles{ layout };

    auto particles = createInterleavedMemoryParticle<2>(source);
    auto view = particles.position();

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

    auto particles = createInterleavedMemoryParticle<2>(source);
    auto view = particles.velocity();

    ASSERT_FLOAT_EQ(view[0].x, 0.5);
    ASSERT_FLOAT_EQ(view[0].y, 0.5);

    ASSERT_FLOAT_EQ(view[1].x, 1.2);
    ASSERT_FLOAT_EQ(view[1].y, 2.6);

    ASSERT_FLOAT_EQ(view[2].x, 0.7);
    ASSERT_FLOAT_EQ(view[2].y, 0.1);
}