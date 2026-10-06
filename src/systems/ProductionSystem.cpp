#include "systems/ProductionSystem.h"

#include "content/Content.h"

namespace game {
void ProductionSystem::tick(GameState& state, double dt) {
    // Passive income: most valued in later eras, mvp =0;
    const std::size_t n = state.resources.size();
    for (std::size_t i = 0; i < n; ++i) {
        state.resources.addByIdx(i, state.resources.rateAt(i) * dt);
    }
}

double ProductionSystem::click(GameState& state) const {
    const double gain = computeClickPower(state);
    const auto& defs = content::Content::instance().allResources();
    for (std::size_t i = 0; i < defs.size() && i < state.resources.size();
         ++i) {
        if (defs[i].clickYield > 0.0) {
            state.resources.addByIdx(i, defs[i].clickYield * gain);
        }
    }
    return gain;
}

double ProductionSystem::computeClickPower(const GameState& state) const {
    return state.clickPower * state.qualityMult * state.speedMult;
}
}  // namespace game
