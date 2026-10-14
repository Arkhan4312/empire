#pragma once
#include <empire/game/state/GameState.h>
#include <empire/engine/core/Tick.h>
#include <empire/game/systems/ArmySystem.h>
#include <empire/game/systems/EconomySystem.h>
#include <empire/game/systems/EraSystem.h>
#include <empire/game/systems/ProductionSystem.h>
#include <empire/game/systems/UpgradeSystem.h>

namespace game {
// Actual gameplay core. Main actions, buying/clicking/upgrading/
class GameLogic {
public:
    GameLogic();

    void tick(GameState& state, const TickContext& ctx);

    bool clickMain(GameState& state);
    bool craftUnit(GameState& state, const std::string& unitId, int count = 1);
    double armyDps(const GameState& s) const {
        return m_army.computeArmyDps(s);
    }
    bool buyUpgrade(GameState& state, const std::string& upgradeId);
    bool advanceEra(GameState& state);

    bool build(GameState& state, const std::string& builingId, int count = 1);
    double clickPower(const GameState& state) const {
        return m_production.computeClickPower(state);
    }

    bool buyCheapestUpgrade(GameState& state);

private:
    ProductionSystem m_production;
    ArmySystem m_army;
    UpgradeSystem m_upgrade;
    EraSystem m_era;
    EconomySystem m_economy;
};
}  // namespace game