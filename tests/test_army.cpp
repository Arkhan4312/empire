#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "content/Content.h"
#include "core/GameLogic.h"
#include "core/GameState.h"

using namespace game;
using Catch::Approx;

TEST_CASE("Army: craft spends and adds", "[army]") {
    GameState s;
    content::initNewGame(s);
    s.resources.add(ResourceType::PLASTIC, 10.0);
    GameLogic l;
    REQUIRE(l.craftUnit(s, "soldier_crooked", 1));
    REQUIRE(s.resources.get(ResourceType::PLASTIC) == Approx(5.0));
    REQUIRE(s.unitCount("soldier_crooked") == 1);
}

TEST_CASE("Army: craft fails without resources", "[army]") {
    GameState s;
    content::initNewGame(s);
    GameLogic l;
    REQUIRE_FALSE(l.craftUnit(s, "soldier_crooked", 1));
    REQUIRE(s.unitCount("soldier_crooked") == 0);
}

TEST_CASE("Army: DPS sums across stacks", "[army]") {
    GameState s;
    content::initNewGame(s);
    s.units.push_back({"soldier_crooked", 3});  // dmg 1
    s.units.push_back({"tank_matchbox", 2});    // dmg 3
    ArmySystem a;
    REQUIRE(a.computeArmyDps(s) == Approx(3.0 + 6.0));
}

TEST_CASE("Army: tick damages boss", "[army]") {
    GameState s;
    content::initNewGame(s);
    s.units.push_back({ "soldier_crooked",10 });
    GameLogic l;
    TickContext ctx{1.0};
    const double hpBefore = s.currentBoss.hp;
    l.tick(s, ctx);
    REQUIRE(s.currentBoss.hp == Approx(hpBefore - 10.0));
}

TEST_CASE("Army: boss death advances index", "[army]") {
    GameState s;
    content::initNewGame(s);
    s.units.push_back({"tank_matchbox", 100});  // 300 dps
    GameLogic l;
    TickContext ctx{1.0};
    l.tick(s, ctx);
    REQUIRE(s.bossIndex == 1);
}