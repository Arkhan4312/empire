#pragma once
#include <string>
#include <vector>

#include "data/ResourceDef.h"

namespace game {
// Units definiton
struct ResourceAmount {
    ResourceId id = kInvalidResource;
    double amount = 0.0;
};
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
};

}  // namespace game