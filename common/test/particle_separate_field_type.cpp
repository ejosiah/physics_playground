#include "particle_type_fixture.h"

TEST_F(ParticleTypeFixture, SeparateFieldMemoryLayoutPositionView) {
    std::vector<glm::vec2> positions{glm::vec2{0}, glm::vec2{1}, glm::vec3{3}};
    SeparateFieldParticle2D particles = createSeparateFieldParticle2D( positions, {}, {}, {}, {} );

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

TEST_F(ParticleTypeFixture, SeparateFieldMemoryLayoutVelocityView) {
    std::vector<glm::vec2> velocity{ glm::vec3(0.5), glm::vec2(1.2, 2.6),  glm::vec2(0.7, 0.1)};

    SeparateFieldParticle2D particles = createSeparateFieldParticle2D( {}, velocity, {}, {}, {} );
    auto view = particles.velocity();

    ASSERT_FLOAT_EQ(view[0].x, 0.5);
    ASSERT_FLOAT_EQ(view[0].y, 0.5);

    ASSERT_FLOAT_EQ(view[1].x, 1.2);
    ASSERT_FLOAT_EQ(view[1].y, 2.6);

    ASSERT_FLOAT_EQ(view[2].x, 0.7);
    ASSERT_FLOAT_EQ(view[2].y, 0.1);
}

TEST_F(ParticleTypeFixture, SeparateFieldMemoryLayoutMassView) {
    std::vector<float> inverseMass{0.1, 2.5, 0.01};
    SeparateFieldParticle2D particles = createSeparateFieldParticle2D( {}, {}, inverseMass, {}, {} );
    auto view = particles.inverseMass();

    ASSERT_FLOAT_EQ(view[0], 0.1);
    ASSERT_FLOAT_EQ(view[1], 2.5);
    ASSERT_FLOAT_EQ(view[2], 0.01);
}

TEST_F(ParticleTypeFixture, SeparateFieldMemoryLayoutRestitutionView) {
    std::vector<float> restitution{ 0.8, 0.99, 0.5};
    SeparateFieldParticle2D particles = createSeparateFieldParticle2D( {}, {}, {}, restitution, {} );

    auto view = particles.restitution();

    ASSERT_FLOAT_EQ(view[0], 0.8);
    ASSERT_FLOAT_EQ(view[1], 0.99);
    ASSERT_FLOAT_EQ(view[2], 0.5);
}

TEST_F(ParticleTypeFixture, SeparateFieldMemoryLayoutRadiusView) {
    std::vector<float> radius{ 1.0, 2.5, 0.123 };
    SeparateFieldParticle2D particles = createSeparateFieldParticle2D( {}, {}, {}, {}, radius );

    auto view = particles.radius();

    ASSERT_FLOAT_EQ(view[0], 1.0);
    ASSERT_FLOAT_EQ(view[1], 2.5);
    ASSERT_FLOAT_EQ(view[2], 0.123);
}