#pragma once
#include <string>
#include <vector>

#include <empire/game/data/ResourceDef.h>

namespace game {

struct UnitDef {
    // Base
    std::string id;
    std::string name;
    // Resource costs
    std::vector<ResourceAmount> costs;
    // Stats
    double damage = 1.0;
    double hp = 1.0;
    double speed = 1.0;
    bool unlockedByDefault = true;
};

}  // namespace game