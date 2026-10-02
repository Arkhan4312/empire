#pragma once
#include <functional>
#include <string>

namespace game {
struct GameState;  // fwd
// Upgrade definition
struct UpgradeDef {
    // Base
    std::string id;
    std::string name;
    // Upgrade resource costs
    double costPlastic = 0.0;
    double costPaper = 0.0;
    double costGlue = 0.0;
    // Levels
    int maxLevel = 1;
    // Apply gameState
    std::function<void(GameState&)> apply;
};
}  // namespace game