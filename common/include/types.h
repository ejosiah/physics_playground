#pragma once

template<typename T>
auto to(auto from) {
    return static_cast<T>(from);
}

template<typename T>
auto as(auto from) {
    return reinterpret_cast<T*>(from);
}