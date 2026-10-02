#pragma once
#include <string>

namespace game {
// Units definiton
struct UnitDef {
    // Base
    std::string id;
    std::string name;
    // Resource costs
    double costPlastic = 0.0;
    double costPaper = 0.0;
    double costGlue = 0.0;
    // Stats
    double damage = 1.0;
    double hp = 1.0;
    double speed = 1.0;
};

}  // namespace game