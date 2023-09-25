#pragma once

#include <glm/glm.hpp>
#include <random>
#include <array>
#include <tuple>
#include <spdlog/spdlog.h>
#include <glm_format.h>

template<glm::length_t L>
struct Bounds {
    using vecType = glm::vec<L, float, glm::defaultp>;
    vecType lower{};
    vecType upper{};

    Bounds() = default;

    Bounds(vecType lc, vecType uc)
    : lower{lc}
    , upper{uc}
    {}

    Bounds(float lc, float uc)
    : lower{ lc }
    , upper{ uc }
    {}
};


template<glm::length_t L>
auto random(Bounds<L>& bounds, uint32_t seed = std::random_device{}()){
    std::array<std::uniform_real_distribution<float>, L> dist;

    for(auto i = 0; i < L; i++){
        dist[i] = {bounds.lower[i], bounds.upper[i]};
    }
    std::default_random_engine engine{seed};
    return [dist, engine]() mutable {
        decltype(bounds.lower) result{};
        for(auto i = 0; i < L; i++){
            result[i] = dist[i](engine);
        }
        return result;
    };
}

template<glm::length_t L>
auto dimensions(const Bounds<L>& bounds){
    auto [lower, upper] = bounds;
    auto dim = upper - lower;
    return std::make_tuple(dim.x, dim.y);
}

template<glm::length_t L>
Bounds<L> shrink(const Bounds<L>& bounds, float factor){
    Bounds<L> newBounds = bounds;
    newBounds.lower += factor;
    newBounds.upper -= factor;

    return newBounds;
}

template<glm::length_t L>
Bounds<L> expand(const Bounds<L>& bounds, float factor){
    Bounds<L> newBounds = bounds;
    newBounds.lower -= factor;
    newBounds.upper += factor;

    return newBounds;
}

template<glm::length_t L>
auto volume(const Bounds<L>& bounds) {
    auto dim = bounds.upper - bounds.lower;
    auto res = dim.x * dim.y;
    if constexpr (L == 3) {
        res *= dim.z;
    }

    return res;
}