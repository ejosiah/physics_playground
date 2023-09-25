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

struct PrimeHash {

    int32_t operator()(glm::ivec2 pid) const {
        return 541 * pid.x + 79 * pid.y;
    }

    int32_t operator()(glm::ivec3 pid) const {
        return 541 * pid.x + 79 * pid.y + 31 * pid.z;
    }

};

template<glm::length_t L, bool Unbounded = true, typename Hash = PrimeHash>
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
    SpacialHashGrid2D(float spacing, glm::vec<L, int> gridSize)
    : m_spacing(spacing)
    , m_gridSize(gridSize)
    , m_tableSize(glm::floor((gridSize.x/spacing * gridSize.y/spacing)))
    , m_counts(glm::floor(gridSize.x/spacing * gridSize.y/spacing) + 1)
    , m_cellEntries(glm::floor(gridSize.x/spacing * gridSize.y/spacing))
    , m_queryIds(glm::floor(gridSize.x/spacing * gridSize.y/spacing))
    , m_querySize(0)
    {}


    [[nodiscard]]
    int32_t hashPosition(glm::vec<L, float> position) const {
        auto pid = intCoords(position);
        if constexpr (!Unbounded){
            auto gridSize = glm::vec<L, int>(glm::vec<L, float>(m_gridSize) / m_spacing);
            pid = glm::clamp(pid, glm::vec<L, int>(0), gridSize - 1);
        }
       return hash(pid);
    }

    void initialize(std::span<glm::vec<L, float>> positions) {
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

    template<typename = std::enable_if<L == 2>>
    [[nodiscard]] int32_t hash(glm::vec<L, int> pid) const {
        static Hash hash{};
        if constexpr (Unbounded) {
            return glm::abs(hash(pid)) % static_cast<int32_t>(m_tableSize);
        } else {
            return pid.x + (m_gridSize.x/m_spacing * pid.y);  // TODO account for z axis
        }
    }


    template<template<typename> typename Layout = SeparateFieldMemoryLayout>
    void initialize(Particle2D<Layout>& particles, size_t size) {
        static int id = -1;
        const auto positions = particles.position();

        try {
            const auto numObjects = glm::min(size, m_cellEntries.size());
            m_set.reset();
            std::fill_n(m_counts.begin(), m_counts.size(), 0);
            std::fill_n(m_cellEntries.begin(), m_cellEntries.size(), 0);
            for (auto i = 0; i < numObjects; i++) {
                id = i;
                auto h = hashPosition(positions[i]);
                m_counts[h]++;
            }

            auto first = m_counts.begin();
            auto last = m_counts.end();
            std::advance(last, -1);
            std::partial_sum(first, last, first);

            m_counts[m_tableSize] = m_counts[m_tableSize - 1];

            auto gridSize = glm::vec<L, int>(glm::vec<L, float>(m_gridSize) / m_spacing);
            for (auto i = 0; i < numObjects; i++) {

                const auto h = hashPosition(positions[i]);
                id = i;
                m_set.set(h);
                m_counts[h]--;
                this->m_cellEntries[this->m_counts[h]] = i;
            }
        }catch(...){
            spdlog::error("error during grid initialization position: {}, hash: {}", positions[id], hashPosition(positions[id]));
            throw;
        }

    }

    [[nodiscard]]
    glm::vec<L, int> intCoords(glm::vec<L, float> position) const {
        return glm::floor(position / m_spacing);
    }

    std::span<int32_t> query(glm::vec<L, float> position, glm::vec<L, float> maxDist) {
        try {
//            auto h0 = hashPosition(position);
//            if (!m_set.test(h0)) {
//                return {};
//            }

            auto d0 = intCoords(position - maxDist);
            auto d1 = intCoords(position + maxDist);

            if constexpr (!Unbounded) {
                d0 = glm::max(glm::vec<L, int>(0), d0);

                auto limit = glm::vec<L, int>(glm::vec<L, float>(m_gridSize) / m_spacing) - 1;
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
//                    auto offset = std::max(0, size - m_cellCapacity);
                    auto offset = 0;
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

    void checkCollision(int gridId, int hash){
        if(!m_collisions.contains(hash)){
            m_collisions[hash] = std::set<int>{};
        }
        if(!m_collisions[hash].contains(gridId)){
            m_collisions[hash].insert(gridId);
        }
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
    const std::vector<int32_t>& entries() const { return m_cellEntries; }

    [[nodiscard]]
    std::vector<int32_t> queryIds() const { return m_queryIds; }

    [[nodiscard]]
    const std::vector<int32_t>& counts() const { return m_counts; }

    std::unordered_map<int, std::set<int>> m_collisions;


private:
    float m_spacing{};
    uint32_t m_tableSize{};
    std::vector<int32_t> m_counts{};
    std::vector<int32_t> m_cellEntries{};
    std::vector<int32_t> m_queryIds{};
    uint32_t m_querySize{};
    int32_t m_cellCapacity{4};
    glm::vec<L, int> m_gridSize{};
    std::bitset<1000000> m_set;
};


using BoundedSpacialHashGrid2D = SpacialHashGrid2D<2, false>;
using UnBoundedSpacialHashGrid2D = SpacialHashGrid2D<2, true>;

using BoundedSpacialHashGrid3D = SpacialHashGrid2D<3, false>;
using UnBoundedSpacialHashGrid3D = SpacialHashGrid2D<3, true>;