#include "core/GameLogic.h"

namespace game {
GameLogic::GameLogic() = default;

void GameLogic::tick(GameState& state, const TickContext& ctx) {
    if (ctx.dt <= 0.0) {
        return;
    }

    state.playTimeSeconds += static_cast<std::int64_t>(ctx.dt);

    m_production.tick(state, ctx.dt);
    m_army.tick(state, ctx.dt);
    m_era.checkTransition(state);
}

bool GameLogic::clickMain(GameState& state) {
    m_production.click(state);
    return true;
}

bool GameLogic::craftUnit(GameState& state, const std::string& unitId, int count) {
    return m_army.craft(state, unitId, count);
}

bool GameLogic::buyUpgrade(GameState& state, const std::string& upgradeId) {
    return m_upgrade.buy(state, upgradeId);
}

bool GameLogic::advanceEra(GameState& state) {
    return m_era.advanceEra(state);
}
}  // namespace game