#pragma once
#include <cstddef>
#include <string_view>
#include <vector>

#include "core/GameState.h"
#include "data/BossDef.h"
#include "data/UnitDef.h"
#include "data/UpgradeDef.h"

namespace game::content {

const UnitDef* findUnit(std::string_view id);
const UpgradeDef* findUpgrade(std::string_view id);
const BossDef* bossAt(int index);
std::size_t bossCount();

const std::vector<UnitDef>& allUnits();
const std::vector<UpgradeDef>& allUpgrades();
const std::vector<BossDef>& allBosses();
//initialize fresh game
void initNewGame(GameState& state);
}  // namespace game::content