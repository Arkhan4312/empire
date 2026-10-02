#include "systems/ProductionSystem.h"

namespace game {
void ProductionSystem::tick(GameState& state, double dt) {
    // Passive income: most valued in later eras, mvp =0;

    for (std::size_t i = 0; i < kResourceCount; ++i) {
        state.resources.amount[i] += state.resources.rate[i] * dt;
    }
    (void)state;
}

double ProductionSystem::click(GameState& state) const {
    const double gain = computeClickPower(state);
    state.resources.add(ResourceType::PLASTIC, gain);
    return gain;
}

double ProductionSystem::computeClickPower(const GameState& state) const {
    return state.clickPower * state.qualityMult * state.speedMult;
}
}  // namespace game
