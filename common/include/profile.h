#pragma once

#include <chrono>

namespace chrono = std::chrono;

template<typename TimeUnit, typename Function>
inline TimeUnit profile(Function&& function){
    auto start = chrono::steady_clock::now();
    function();
    auto duration = chrono::steady_clock::now() - start;
    return chrono::duration_cast<TimeUnit>(duration);
}