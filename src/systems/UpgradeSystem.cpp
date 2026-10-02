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

    if (state.resources.get(ResourceType::PLASTIC) < def->costPlastic) {
        return false;
    }

    if (state.resources.get(ResourceType::PAPER) < def->costPaper) {
        return false;
    }

    if (state.resources.get(ResourceType::GLUE) < def->costGlue) {
        return false;
    }

    state.resources.spend(ResourceType::PLASTIC, def->costPlastic);
    state.resources.spend(ResourceType::PAPER, def->costPaper);
    state.resources.spend(ResourceType::GLUE, def->costGlue);

    if (cur) {
        cur->level += 1;
    } else {
        state.upgrades.push_back(UpgradeState{upgradeId, 1});
    }

    if (def->apply) {
        def->apply(state);
    }
    return true;
}
}  // namespace game