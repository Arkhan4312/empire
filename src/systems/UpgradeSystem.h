#pragma once
#include <string>

#include "core/GameState.h"

namespace game {
class UpgradeSystem {
public:
    bool buy(GameState& state, const std::string& upgradeId);
};
}  // namespace game