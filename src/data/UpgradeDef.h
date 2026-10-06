#pragma once
#include <functional>
#include <string>
#include <vector>
namespace game {
struct GameState;  // fwd

struct EffectRef {
    std::function<void(GameState&, double)> fn;
    double value = 0.0;
};

// Upgrade definition
struct UpgradeDef {
    // Base
    std::string id;
    std::string name;
    // Upgrade resource costs
    std::vector<ResourceAmount> costs;
    // Levels
    int maxLevel = 1;
    std::vector<EffectRef> effects;
    // Apply gameState
    void apply(GameState& s) const {
        for (const auto& e : effects) {
            if (e.fn) {
                e.fn(s, e.value);
            }
        }
    }
};
}  // namespace game