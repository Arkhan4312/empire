#include <catch2/catch_test_macros.hpp>

#include "content/Content.h"
#include "core/GameLogic.h"
#include "core/GameState.h"

using namespace game;

TEST_CASE("Era: transition after defeating all bosses", "[era]") {
    GameState s;
    content::initNewGame(s);
    REQUIRE(s.era == Era::CHILDHOOD);

    // Kill boss 0;
    s.currentBoss.hp = 0.1;
    s.units.push_back({"tank_matchbox", 100});
    GameLogic l;
    l.tick(s, TickContext{1.0});
    REQUIRE(s.bossIndex == 1);

    // Kill boss 1;
    s.currentBoss.hp = 0.1;
    l.tick(s, TickContext{1.0});
    REQUIRE(s.bossIndex == 2);

    // Kill boss 2;
    s.currentBoss.hp = 0.1;
    l.tick(s, TickContext{1.0});
    // should advance
    REQUIRE(s.era == Era::ENTREPRENEUR);
}

TEST_CASE("Era: advanceEra no-op if already in era 2 (at least)", "[era]") {
    GameState s;
    content::initNewGame(s);
    EraSystem e;
    e.advanceEra(s);
    REQUIRE_FALSE(e.advanceEra(s));  // no-op
}