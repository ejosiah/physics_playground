#pragma once

#include "snap.h"
#include "particle.h"
#include <glm/glm.hpp>
#include <fmt/format.h>
#include <boost/functional/hash.hpp>
#include <vector>
#include <algorithm>
#include <numeric>
#include <span>
#include <type_traits>
#include <bitset>
#include <glm_format.h>

template<bool Unbounded = true>
class SpacialHashGrid2D {
public:
    SpacialHashGrid2D() = default;

    template<typename  = std::enable_if<Unbounded>>
    SpacialHashGrid2D(float spacing, int32_t maxNumObjects)
    : m_spacing(spacing)
    , m_tableSize(2 * maxNumObjects)
    , m_counts(2 * maxNumObjects + 1)
    , m_cellEntries(maxNumObjects)
    , m_queryIds(maxNumObjects)
    , m_querySize(0)
    {}

    template<typename = std::enable_if<!Unbounded>>
    SpacialHashGrid2D(float spacing, glm::ivec2 gridSize)
    : m_spacing(spacing)
    , m_gridSize(gridSize)
    , m_tableSize(glm::floor((gridSize.x/spacing * gridSize.y/spacing)))
    , m_counts(glm::floor(gridSize.x/spacing * gridSize.y/spacing) + 1)
    , m_cellEntries(glm::floor(gridSize.x/spacing * gridSize.y/spacing))
    , m_queryIds(glm::floor(gridSize.x/spacing * gridSize.y/spacing))
    , m_querySize(0)
    {}


    [[nodiscard]]
    int32_t hashPosition(glm::vec2 position) const {
        auto pid = intCoords(position);
       return hash(pid);
    }

    [[nodiscard]]
    int32_t hash(glm::ivec2 pid) const {
        if constexpr (Unbounded) {
            const auto h = (pid.x * 92837111) ^ (pid.y * 689287499);
            return glm::abs(h) % static_cast<int32_t>(m_tableSize);
        } else {
            return pid.x + (m_gridSize.x/m_spacing * pid.y);
        }
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
    void initialize(Particle2D<Layout>& particles, size_t size) {
        const auto numObjects = glm::min(size, m_cellEntries.size());
        m_set.reset();
        std::fill_n(m_counts.begin(), m_counts.size(), 0);
        std::fill_n(m_cellEntries.begin(), m_cellEntries.size(), 0);
        const auto positions = particles.position();
        for (auto i = 0; i < numObjects; i++) {
            auto h = hashPosition(positions[i]);
//                spdlog::info("pos: {}, hash: {}", positions[i], h);
            m_counts[h]++;
        }

        auto first = m_counts.begin();
        auto last = m_counts.end();
        std::advance(last, -1);
        std::partial_sum(first, last, first);

        m_counts[m_tableSize] = m_counts[m_tableSize - 1];

        auto gridSize = glm::ivec2(glm::vec2(m_gridSize) / m_spacing);
        for (auto i = 0; i < numObjects; i++) {
            auto pid = intCoords(positions[i]);
            if(pid.x < 0 || pid.x > gridSize.x || pid.y < 0 || pid.y > gridSize.y){
                spdlog::info("point {} {} is outside of grid", positions[i], glm::vec2(pid));
            }
            const auto h = hashPosition(positions[i]);
            m_set.set(h);
            m_counts[h]--;
//                assert(m_counts[h] >= 0);
            this->m_cellEntries[this->m_counts[h]] = i;
        }

    }

    [[nodiscard]]
    glm::ivec2 intCoords(glm::vec2 position) const {
        return glm::floor(position / m_spacing);
    }

    std::span<int32_t> query(glm::vec2 position, glm::vec2 maxDist) {
        try {
            auto h0 = hashPosition(position);
            if (!m_set.test(h0)) {
                return {};
            }

            auto d0 = intCoords(position - maxDist);
            auto d1 = intCoords(position + maxDist);

            if constexpr (!Unbounded) {
                d0 = glm::max(glm::ivec2(0), d0);

                auto limit = glm::ivec2(glm::vec2(m_gridSize) / m_spacing) - 1;
                d1 = glm::min(limit, d1);
            }

            std::fill_n(m_queryIds.begin(), m_querySize, 0);
            m_querySize = 0;

            for (auto xi = d0.x; xi <= d1.x; ++xi) {
                for (auto yi = d0.y; yi <= d1.y; ++yi) {
                    const auto h = hash({xi, yi});
                    auto start = m_counts[h];
                    const auto end = m_counts[h + 1];

                    auto size = end - start;
                    auto offset = std::max(0, size - m_cellCapacity);
                    start += offset;

                    for (auto i = start; i < end; ++i) {
                        this->m_queryIds[m_querySize] = m_cellEntries[i];
                        m_querySize++;
                    }
                }
            }
        }catch(...){
            spdlog::error("error processing position: {}", position);
            throw;
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
    int32_t m_cellCapacity{4};
    glm::ivec2 m_gridSize{};
    std::bitset<1000000> m_set;
};


using BoundedSpacialHashGrid2D = SpacialHashGrid2D<false>;
using UnBoundedSpacialHashGrid2D = SpacialHashGrid2D<true>;