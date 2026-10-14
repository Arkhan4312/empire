#pragma once
#include <empire/game/state/GameState.h>

namespace game {

class ProductionSystem {
public:
    double click(GameState& state) const;
    double computeClickPower(const GameState& state) const;
};
}  // namespace game