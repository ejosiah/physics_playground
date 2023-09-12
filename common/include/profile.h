#pragma once

#include <chrono>
#include <algorithm>
#include <numeric>

namespace chrono = std::chrono;

using Average = float;

template<typename TimeUnit, typename Function>
inline TimeUnit profile(Function&& function){
    auto start = chrono::high_resolution_clock::now();
    function();
    auto duration = chrono::high_resolution_clock::now() - start;
    return chrono::duration_cast<TimeUnit>(duration);
}

template<typename TimeUnit, typename Setup, typename Body>
inline Average profile(Setup&& setup, Body&& body, size_t runs) {
    setup();
    std::vector<TimeUnit> runtimes(runs);
    for(auto i = 0; i < runs; ++i) {
        auto runtime = profile(body);
        runtimes[i] = runtime.count();
    }

    auto sum = std::accumulate(runtimes.begin(), runtimes.end(), TimeUnit{});
    return static_cast<Average>(sum)/static_cast<Average>(runs);
}