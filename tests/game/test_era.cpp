#include <empire/game/content/Content.h>
#include <empire/game/logic/GameLogic.h>
#include <empire/game/state/GameState.h>
#include <empire/game/systems/EraSystem.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace game;
using Catch::Approx;

TEST_CASE("Era: checkTransition no-op mid-era", "[era]") {
    GameState s;
    content::initNewGame(s);
    EraSystem e;
    e.checkTransition(s);
    REQUIRE(s.era == Era::CHILDHOOD);
}

TEST_CASE("Era: checkTransition advances when bossIndex exhausted", "[era]") {
    GameState s;
    content::initNewGame(s);
    const int total = content::Content::instance().bossCount(Era::CHILDHOOD);
    REQUIRE(total > 0);

    s.bossIndex = total;
    EraSystem e;
    e.checkTransition(s);
    REQUIRE(s.era == Era::ENTREPRENEUR);
}

TEST_CASE("Era: advanceEra on ENTREPRENEUR returns false", "[era]") {
    GameState s;
    content::initNewGame(s);
    EraSystem e;
    REQUIRE(e.advanceEra(s));
    REQUIRE(s.era == Era::ENTREPRENEUR);
    REQUIRE_FALSE(e.advanceEra(s));
    REQUIRE(s.era == Era::ENTREPRENEUR);
}

TEST_CASE("Era: full defeat chain reaches ENTREPRENEUR in a single tick",
          "[era][!shouldfail]") {
    // Due to the double-overflow bug, killing all bosses in one tick is
    // anyway achievable, but this test asserts the semantic "all bosses dead
    // => era 2" — separate from the overflow bug.
    GameState s;
    content::initNewGame(s);
    s.units.push_back({"tank_matchbox", 1000});  // 3000 dps => overkill
    GameLogic l;
    l.tick(s, TickContext{1.0});
    REQUIRE(s.era == Era::ENTREPRENEUR);
}