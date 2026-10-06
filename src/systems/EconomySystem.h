#pragma once
#include "core/GameState.h"

namespace game {

class EconomySystem {
public:
    void tick(GameState& state, double dt);
    bool build(GameState& state, const std::string& buildingId, int count = 1);
    double nextCost(const GameState& state, const std::string& buildingId, ResourceId resourceId) const;
    double totalRate(const GameState& state, std::size_t resourceIdx) const;
};
}  // namespace game