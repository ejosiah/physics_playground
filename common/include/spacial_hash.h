#pragma once

#include <vector>
#include <algorithm>
#include <numeric>
#include <glm/glm.hpp>
#include <span>
#include <fmt/format.h>
#include "particle.h"

class SpacialHashGrid2D {
public:
    SpacialHashGrid2D() = default;

    SpacialHashGrid2D(float spacing, int32_t maxNumObjects)
    : m_spacing(spacing)
    , m_tableSize(2 * maxNumObjects)
    , m_counts(2 * maxNumObjects + 1)
    , m_cellEntries(maxNumObjects)
    , m_queryIds(maxNumObjects)
    , m_querySize(0)
    {}


    [[nodiscard]]
    int32_t hashPosition(glm::vec2 position) const {
        return hashCoords(intCoords(position));
    }

    [[nodiscard]]
    int32_t hashCoords(glm::ivec2 pid) const {
        const auto h = (pid.x * 92837111) ^ (pid.y * 689287499);	// fantasy function
        return glm::abs(h) % static_cast<int32_t>(m_tableSize);
    }

    void initialize(std::span<glm::vec2> positions) {
        const auto numObjects = glm::min(positions.size(), m_cellEntries.size());

        std::fill_n(m_counts.begin(), m_counts.size(), 0);
        std::fill_n(m_cellEntries.begin(), m_cellEntries.size(), 0);

        for(auto i = 0; i < numObjects; i++){
            auto h = hashPosition(positions[i]);
            m_counts[h]++;
        }

        auto first = m_counts.begin();
        auto last = m_counts.end();
        std::advance(last, -1);
        std::partial_sum(first, last, first);

        m_counts[m_tableSize] = m_counts[m_tableSize - 1];

        for(auto i = 0; i < numObjects; i++){
            const auto h = hashPosition(positions[i]);
            m_counts[h]--;
            this->m_cellEntries[this->m_counts[h]] = i;
        }
    }

    template<template<typename> typename Layout>
    void initialize(Particle2D<Layout>& particles) {
        const auto numObjects = glm::min(particles.layout.size, m_cellEntries.size());

        std::fill_n(m_counts.begin(), m_counts.size(), 0);
        std::fill_n(m_cellEntries.begin(), m_cellEntries.size(), 0);
        const auto positions = particles.position();
        for(auto i = 0; i < numObjects; i++){
            auto h = hashPosition(positions[i]);
            m_counts[h]++;
        }

        auto first = m_counts.begin();
        auto last = m_counts.end();
        std::advance(last, -1);
        std::partial_sum(first, last, first);

        m_counts[m_tableSize] = m_counts[m_tableSize - 1];

        for(auto i = 0; i < numObjects; i++){
            const auto h = hashPosition(positions[i]);
            m_counts[h]--;
            this->m_cellEntries[this->m_counts[h]] = i;
        }
    }

    [[nodiscard]]
    glm::ivec2 intCoords(glm::vec2 position) const {
        return glm::floor(position / m_spacing);
    }

    std::span<int32_t> query(glm::vec2 position, glm::vec2 maxDist) {
        auto d0 = intCoords(position - maxDist);
        auto d1 = intCoords(position + maxDist);

        std::fill_n(m_queryIds.begin(), m_querySize, 0);
        m_querySize = 0;

        for(auto xi = d0.x; xi <= d1.x; ++xi){
            for(auto yi = d0.y; yi <= d1.y; ++yi){
                const auto h = hashCoords({xi, yi});
                const auto start = m_counts[h];
                const auto end = m_counts[h + 1];

                for(auto i = start; i < end; ++i){
                    this->m_queryIds[m_querySize] = m_cellEntries[i];
                    m_querySize++;
                }
            }
        }

        return queryResults();
    }

    [[nodiscard]]
    inline std::span<int32_t> queryResults() {
        return std::span{ m_queryIds.data(), m_querySize };
    }

    [[nodiscard]]
    float numSpacing() const { return m_spacing; }

    [[nodiscard]]
    int32_t size() const { return m_tableSize; }

    [[nodiscard]]
    std::vector<int32_t> entries() const { return m_cellEntries; }

    [[nodiscard]]
    std::vector<int32_t> queryIds() const { return m_queryIds; }

    [[nodiscard]]
    std::vector<int32_t> counts() const { return m_counts; }

private:
    float m_spacing{};
    uint32_t m_tableSize{};
    std::vector<int32_t> m_counts{};
    std::vector<int32_t> m_cellEntries{};
    std::vector<int32_t> m_queryIds{};
    uint32_t m_querySize{};
};
