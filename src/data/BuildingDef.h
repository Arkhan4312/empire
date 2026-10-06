#pragma once
#include <string>
#include <vector>

#include "data/ResourceDef.h"

namespace game {

struct BuildingDef {
    std::string id;
    std::string name;
    std::vector<ResourceAmount> costs;
    std::vector<ResourceAmount> production;
    double costMultiplier = 1.15;
    bool unlockedByDefault = true;
};
}  // namespace game