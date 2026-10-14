#pragma once
#include <string>

#include <empire/game/state/GameState.h>

namespace game {
class UpgradeSystem {
public:
    bool buy(GameState& state, const std::string& upgradeId);
};
}  // namespace game