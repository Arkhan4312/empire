#include "content/Content.h"

#include <algorithm>
#include <vector>

namespace game::content {

// non-class static functions
//  UNITS
static std::vector<UnitDef> makeUnits() {
    return {
        {"soldier_crooked", "Солдатик с кривыми руками", 5, 0, 0, 1, 5, 1.0},
        {"tank_matchbox", "Танк из спичечного коробка", 25, 0, 0, 3, 20, 0.5},
        {"plane_paper", "Бумажный самолётик", 0, 10, 0, 2, 3, 3.0},
    };
}
// UPGRADES
static std::vector<UpgradeDef> makeUpgrades() {
    std::vector<UpgradeDef> u;

    u.push_back({"up_plastic_1", "Новвый набор пластилина", 100, 0, 0, 5,
                 [](GameState& s) { s.qualityMult *= 1.25; }});
    u.push_back({"up_tools_1", "Папины инструменты", 500, 0, 0, 3,
                 [](GameState& s) { s.qualityMult *= 1.10; }});
    return u;
}
// BOSSES
static std::vector<BossDef> makeBosses() {
    return {
        {"boss_kolya", "Колька из 3-го подъезда", 50, 2},
        {"boss_serega", "Старшеклассник Серёга", 150, 5},
        {"boss_vitka", "Местный задира Витька", 400, 15},
    };
}
// ACCESSORS
const UnitDef* findUnit(std::string_view id) {
    const auto& v = allUnits();
    auto it = std::find_if(v.begin(), v.end(),
                           [id](const UnitDef& u) { return u.id == id; });
    return it == v.end() ? nullptr : &*it;
}

const UpgradeDef* findUpgrade(std::string_view id) {
    const auto& v = allUpgrades();
    auto it = std::find_if(v.begin(), v.end(),
                           [id](const UpgradeDef& u) { return u.id == id; });
    return it == v.end() ? nullptr : &*it;
}

const BossDef* bossAt(int index) {
    const auto& v = allBosses();
    if (index < 0 || static_cast<std::size_t>(index) >= v.size()) {
        return nullptr;
    }
    return &v[static_cast<std::size_t>(index)];
}

std::size_t bossCount() {
    return allBosses().size();
}

const std::vector<UnitDef>& allUnits() {
    static const std::vector<UnitDef> v = makeUnits();
    return v;
}

const std::vector<UpgradeDef>& allUpgrades() {
    static const std::vector<UpgradeDef> v = makeUpgrades();
    return v;
}

const std::vector<BossDef>& allBosses() {
    static const std::vector<BossDef> v = makeBosses();
    return v;
}
// refresh game (Init)
void initNewGame(GameState& state) {
    state = GameState{};
    state.era = Era::CHILDHOOD;
    state.bossIndex = 0;

    if (const BossDef* b = bossAt(0)) {
        state.currentBoss = Boss{b->id, b->name, b->maxHp, b->maxHp, b->dps};
    }
}
}  // namespace game::content