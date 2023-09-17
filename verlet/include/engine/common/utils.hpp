#pragma once
#include <sstream>
#include <glm/glm.hpp>
#include "index_vector.hpp"

template<typename T>
using CIVector = civ::Vector<T>;


template<typename T>
T sign(T v)
{
    return v < 0.0f ? -1.0f : 1.0f;
}


template<typename T>
static std::string toString(T value)
{
    std::stringstream sx;
    sx << value;
    return sx.str();
}

