#include "systems/EraSystem.h"

#include "content/Content.h"

namespace game {
void EraSystem::checkTransition(GameState& state) {
    if (state.era == Era::CHILDHOOD &&
        state.bossIndex >= static_cast<int>(content::bossCount())) {
        advanceEra(state);
    }
}
bool EraSystem::advanceEra(GameState& state) {
    if (state.era == Era::CHILDHOOD) {
        state.era = Era::ENTREPRENEUR;
        state.bossIndex = 0;
        state.currentBoss = Boss{};
        return true;
    }
    return false;
}
}  // namespace game