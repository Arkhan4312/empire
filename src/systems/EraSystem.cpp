#include "systems/EraSystem.h"

#include "content/Content.h"

namespace game {
void EraSystem::checkTransition(GameState& state) {
    const auto& content = content::Content::instance();
    if (state.bossIndex >= content.bossCount(state.era)) {
        advanceEra(state);
    }
}

bool EraSystem::advanceEra(GameState& state) {
    if (state.era == Era::CHILDHOOD) {
        state.era = Era::ENTREPRENEUR;
        state.bossIndex = 0;
        if (const BossDef* b =
                content::Content::instance().bossAt(state.era, 0)) {
            state.currentBoss =
                Boss{b->id, b->name, b->maxHp, b->maxHp, b->dps};
        } else {
            state.currentBoss = Boss{};
        }
        return true;
    }
    return false;
}
}  // namespace game