#pragma once
#include "core/GameState.h"

namespace game {
class ArmySystem {
public:
    void tick(GameState& state, double dt);
    bool craft(GameState& state, const std::string& unitId, int count);
    double computeArmyDps(const GameState& state) const;
    void damageBoss(GameState& state, double dps, double dt);
};
}  // namespace game