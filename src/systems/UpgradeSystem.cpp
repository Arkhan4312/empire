#include "systems/UpgradeSystem.h"

#include "content/Content.h"

namespace game {
bool UpgradeSystem::buy(GameState& state, const std::string& upgradeId) {
    const UpgradeDef* def = content::findUpgrade(upgradeId);
    if (!def) {
        return false;
    }
    UpgradeState* cur = state.findUpgrade(upgradeId);
    const int currentLevel = cur ? cur->level : 0;
    if (currentLevel >= def->maxLevel) {
        return false;
    }

    for (const auto& c : def->costs) {
        if (c.id == kInvalidResource) {
            continue;
        }
        if (state.resources.get(c.id) < c.amount) {
            return false;
        }
    }
    for (const auto& c : def->costs) {
        if (c.id == kInvalidResource) {
            continue;
        }
        state.resources.spend(c.id, c.amount);
    }

    if (cur) {
        cur->level += 1;
    } else {
        state.upgrades.push_back(UpgradeState{upgradeId, 1});
    }

    def->apply(state);
    return true;
}
}  // namespace game