#pragma once
#include "core/GameState.h"

namespace game {

class ProductionSystem {
public:
    double click(GameState& state) const;
    double computeClickPower(const GameState& state) const;
};
}  // namespace game