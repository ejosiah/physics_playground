#pragma once

#include "model2d.h"
#include "particle.h"
#include "spacial_hash.h"
#include "types.h"
#include <span>
#include <vector>

struct Contact {
    int a{-1};
    int b{-1};
    float depth{0};
};

template<template<typename> typename Layout>
class ContactGenerator{
public:

    virtual std::span<Contact> generateContact(Particle2D<Layout>& m_particles) = 0;
};


template<template<typename> typename Layout>
class SpacialHashContactGenerator : public ContactGenerator<Layout> {
public:
    SpacialHashContactGenerator() = default;

    SpacialHashContactGenerator(const Bounds2D& bounds, float maxRadius, int32_t maxNumObjects);

    std::span<Contact> generateContact(Particle2D<Layout> &m_particles) override;

private:
    float m_spacing{};
    std::vector<Contact> m_contacts{};
    Bounds2D m_bounds{};
    SpacialHashGrid2D<> m_grid{};
};


template<template<typename> typename Layout>
SpacialHashContactGenerator<Layout>::SpacialHashContactGenerator(const Bounds2D& bounds, float maxRadius, int32_t maxNumObjects)
: m_spacing{ maxRadius * 2.f }
, m_contacts( maxNumObjects * 10 )
, m_bounds{ shrink(bounds, maxRadius * 2.f) }
, m_grid{ maxRadius * 2.f, maxNumObjects }
{}

template<template<typename> typename Layout>
std::span<Contact> SpacialHashContactGenerator<Layout>::generateContact(Particle2D<Layout> &m_particles) {
    size_t numContacts = 0;
    const auto [width, height] = dimensions(m_bounds);
    const auto iWidth = to<size_t>(glm::floor(width/m_spacing));
    const auto iHeight = to<size_t>(glm::floor(width/m_spacing));
    size_t set = 0;

    return {m_contacts.data(), numContacts };
}

using SpacialHashContactGeneratorSFML = SpacialHashContactGenerator<SeparateFieldMemoryLayout>;