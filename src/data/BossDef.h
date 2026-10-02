#pragma once
#include <string>

namespace game {
// Boss definition. It's actually an enemy/combat unit, but currently only bosses are used for MVP.

struct BossDef {
    // Base
    std::string id;
    std::string name;
    // Stats, only for MVP
    double maxHp = 100.0;
    double dps = 1.0;
};
}  // namespace game