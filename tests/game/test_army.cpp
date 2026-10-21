#include <empire/game/content/Content.h>
#include <empire/game/logic/GameLogic.h>
#include <empire/game/state/GameState.h>
#include <empire/game/systems/ArmySystem.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "test_helpers.h"

using namespace game;
using Catch::Approx;

TEST_CASE("Army: craft spends and adds", "[army]") {
    GameState s;
    content::initNewGame(s);
    s.resources.add(test::plastic(), 10.0);
    GameLogic l;
    REQUIRE(l.craftUnit(s, "soldier_crooked", 1));
    REQUIRE(s.resources.get(test::plastic()) == Approx(5.0));
    REQUIRE(s.unitCount("soldier_crooked") == 1);
}

TEST_CASE("Army: craft fails without resources", "[army]") {
    GameState s;
    content::initNewGame(s);
    GameLogic l;
    REQUIRE_FALSE(l.craftUnit(s, "soldier_crooked", 1));
    REQUIRE(s.unitCount("soldier_crooked") == 0);
}

TEST_CASE("Army: craft of locked unit fails", "[army]") {
    GameState s;
    content::initNewGame(s);
    s.resources.add(test::plastic(), 1000.0);
    s.unlocked.erase("tank_matchbox");
    GameLogic l;
    REQUIRE_FALSE(l.craftUnit(s, "tank_matchbox", 1));
    REQUIRE(s.unitCount("tank_matchbox") == 0);
}

TEST_CASE("Army: DPS sums across stacks", "[army]") {
    GameState s;
    content::initNewGame(s);
    s.units.push_back({"soldier_crooked", 3});  // dmg 1
    s.units.push_back({"tank_matchbox", 2});    // dmg 3
    ArmySystem a;
    REQUIRE(a.computeArmyDps(s) == Approx(3.0 + 6.0));
}

TEST_CASE("Army: DPS is scaled by qualityMult", "[army]") {
    GameState s;
    content::initNewGame(s);
    s.units.push_back({"soldier_crooked", 10});
    s.qualityMult = 2.5;
    ArmySystem a;
    REQUIRE(a.computeArmyDps(s) == Approx(25.0));
}

TEST_CASE("Army: tick damages boss", "[army]") {
    GameState s;
    content::initNewGame(s);
    s.units.push_back({"soldier_crooked", 10});
    GameLogic l;
    const double hpBefore = s.currentBoss.hp;
    l.tick(s, TickContext{1.0});
    REQUIRE(s.currentBoss.hp == Approx(hpBefore - 10.0));
}

TEST_CASE("Army: overflow does NOT double-subtract next boss HP",
          "[army][regression][!shouldfail]") {
    // KNOWN BUG (ArmySystem::damageBoss):
    //   state.currentBoss = Boss{..., next->maxHp - overflow, ...};
    //   remaining = overflow;
    // The next iteration then does `hp -= remaining` again -> overflow is
    // subtracted twice. Correct next-hp = maxHp - overflow.
    GameState s;
    content::initNewGame(s);
    s.currentBoss.hp = 50.0;                   // boss0 fully loaded
    s.units.push_back({"tank_matchbox", 20});  // 60 dps
    GameLogic l;
    l.tick(s, TickContext{1.0});
    REQUIRE(s.bossIndex == 1);
    // Correct: boss1.hp = 150 - (60 - 50) = 140
    // Buggy  : boss1.hp = 150 - 10 - 10 = 130
    REQUIRE(s.currentBoss.hp == Approx(140.0));
}