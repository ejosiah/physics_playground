#pragma once

#include <any>
#include <map>
#include <string>
#include <vector>

struct Snap {

    using Snapshot = std::map<std::string, std::any>;

    template<typename T>
    using Sequence = std::vector<T>;

    virtual  Snapshot snapshot() = 0;
};