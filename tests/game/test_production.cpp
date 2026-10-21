#include <empire/game/content/Content.h>
#include <empire/game/logic/GameLogic.h>
#include <empire/game/state/GameState.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "test_helpers.h"

using namespace game;
using Catch::Approx;

TEST_CASE("Production: click adds plastic", "[production]") {
    GameState s;
    content::initNewGame(s);
    GameLogic l;
    l.clickMain(s);
    REQUIRE(s.resources.get(test::plastic()) == Approx(1.0));
}

TEST_CASE("Production: clickPower depends on quality", "[production]") {
    GameState s;
    content::initNewGame(s);
    s.qualityMult = 2.0;
    GameLogic l;
    l.clickMain(s);
    REQUIRE(s.resources.get(test::plastic()) == Approx(2.0));
}

TEST_CASE("Production: clickPower includes speedMult", "[production]") {
    GameState s;
    content::initNewGame(s);
    s.speedMult = 3.0;
    GameLogic l;
    l.clickMain(s);
    REQUIRE(s.resources.get(test::plastic()) == Approx(3.0));
}

TEST_CASE("Production: computeClickPower combines all multipliers",
          "[production]") {
    GameState s;
    content::initNewGame(s);
    s.clickPower = 2.0;
    s.qualityMult = 3.0;
    s.speedMult = 4.0;
    GameLogic l;
    REQUIRE(l.clickPower(s) == Approx(24.0));
}

// NOTE: manual `setRate` before tick is intentionally wiped by
// EconomySystem::tick (rate is derived from buildings). We test the real
// mechanism via a producing building.
TEST_CASE("Production: building production accumulates over tick",
          "[production]") {
    GameState s;
    content::initNewGame(s);
    s.buildings.push_back({"paper_mill", 1});  // produces 1 paper/s
    GameLogic l;
    l.tick(s, TickContext{10.0});
    REQUIRE(s.resources.get(test::paper()) == Approx(10.0));
}

TEST_CASE("Production: no buildings => zero rate", "[production]") {
    GameState s;
    content::initNewGame(s);
    s.resources.addByIdx(0, 5.0);
    GameLogic l;
    l.tick(s, TickContext{10.0});
    REQUIRE(s.resources.getByIdx(0) == Approx(5.0));  // unchanged
}