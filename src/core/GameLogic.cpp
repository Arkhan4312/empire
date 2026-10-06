#include "GameLogic.h"

#include "content/Content.h"
#include "core/GameLogic.h"
namespace game {
GameLogic::GameLogic() = default;

void GameLogic::tick(GameState& state, const TickContext& ctx) {
    if (ctx.dt <= 0.0) {
        return;
    }

    state.playTimeSeconds += static_cast<std::int64_t>(ctx.dt);

    m_economy.tick(state, ctx.dt);
    m_army.tick(state, ctx.dt);
    m_era.checkTransition(state);
}

bool GameLogic::clickMain(GameState& state) {
    m_production.click(state);
    return true;
}

bool GameLogic::craftUnit(GameState& state, const std::string& unitId,
                          int count) {
    return m_army.craft(state, unitId, count);
}

bool GameLogic::buyUpgrade(GameState& state, const std::string& upgradeId) {
    return m_upgrade.buy(state, upgradeId);
}

bool GameLogic::advanceEra(GameState& state) {
    return m_era.advanceEra(state);
}
bool game::GameLogic::build(GameState& state, const std::string& builingId,
                            int count) {
    return m_economy.build(state, builingId, count);
}
bool game::GameLogic::buyCheapestUpgrade(GameState& state) {
    const auto& C = content::Content::instance();
    const UpgradeDef* best = nullptr;
    double bestCost = 0.0;

    for (const auto& u : C.allUpgrades()) {
        if (!state.isUnlocked(u.id)) {
            continue;
        }
        if (state.upgradeLevel(u.id) >= u.maxLevel) {
            continue;
        }
        double cost = 0.0;
        for (const auto& c : u.costs) {
            if (c.id == kInvalidResource) {
                continue;
            }
            if (state.resources.get(c.id) < c.amount) {
                cost = -1.0;
                break;
            }
            cost += c.amount;
        }
        if (cost < 0.0) {
            continue;
        }
        if (!best || cost < bestCost) {
            best = &u;
            bestCost = cost;
            ;
        }
    }
    if (!best) {
        return false;
    }
    return m_upgrade.buy(state, best->id);
}
}  // namespace game