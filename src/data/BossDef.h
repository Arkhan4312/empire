#pragma once
#include <string>
#include <vector>

#include "data/Era.h"
#include "data/ResourceDef.h"

namespace game {
// Boss definition. It's actually an enemy/combat unit, but currently only
// bosses are used for MVP.
struct GameState;  // fwd

struct BossDef {
    // Base
    std::string id;
    std::string name;
    Era era = Era::CHILDHOOD;
    double maxHp = 100.0;
    double dps = 1.0;

    std::vector<ResourceAmount> reward;
    std::vector<std::string> unlocks;
};
}  // namespace game