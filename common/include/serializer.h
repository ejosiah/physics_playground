#pragma once

#include "snap.h"
#include <map>
#include <string>
#include <any>

namespace yaml {

//    auto serialize(const auto& object) -> std::string;

    auto serialize(const Snap::Snapshot& snapshot) -> std::string;

}