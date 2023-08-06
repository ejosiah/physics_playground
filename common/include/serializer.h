#pragma once

#include <map>
#include <string>
#include <any>

namespace yaml {

    template<typename T>
    auto serialize(const auto& object) -> std::string;

}