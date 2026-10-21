#include <empire/game/content/Content.h>
#include <empire/game/logic/GameLogic.h>
#include <empire/game/state/GameState.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "test_helpers.h"

using namespace game;
using Catch::Approx;

namespace {
void botStep(GameState& s, GameLogic& l) {
    if (s.resources.get(test::plastic()) >= 5.0) {
        l.craftUnit(s, "soldier_crooked", 1);
    } else {
        l.clickMain(s);
    }
}
}  // namespace

TEST_CASE("Balance: bot reaches Era 2 within reasonable simulated time",
          "[balance]") {
    GameState s;
    content::initNewGame(s);
    GameLogic l;

    constexpr double kDt = 0.1;
    constexpr int kMaxTicks = 30000;

    int tick = 0;
    for (; tick < kMaxTicks; ++tick) {
        botStep(s, l);
        l.tick(s, TickContext{kDt});
        if (s.era == Era::ENTREPRENEUR) {
            break;
        }
    }
    INFO("Ticks taken: " << tick);
    INFO("Boss index: " << s.bossIndex);
    INFO("Era: " << static_cast<int>(s.era));
    REQUIRE(s.era == Era::ENTREPRENEUR);
}

TEST_CASE("Balance: bot never causes negative resources", "[balance]") {
    GameState s;
    content::initNewGame(s);
    GameLogic l;

    for (int i = 0; i < 100000; ++i) {
        botStep(s, l);
        l.tick(s, TickContext{0.001});
        for (std::size_t r = 0; r < s.resources.size(); ++r) {
            REQUIRE(s.resources.getByIdx(r) >= 0.0);
            REQUIRE(std::isfinite(s.resources.getByIdx(r)));
        }
    }
}

// Regression: Content::installFallback() passes `true`/`false` into the
// `costMultiplier` slot of BuildingDef. paper_mill ends up with 1.0 (no
// escalation), glue_pot with 0.0 (becomes free after 1 purchase).
TEST_CASE("Balance: paper_mill cost escalates",
          "[balance][regression][!shouldfail]") {
    GameState s;
    content::initNewGame(s);
    s.resources.add(test::plastic(), 10000.0);
    GameLogic l;

    REQUIRE(l.build(s, "paper_mill"));
    const double afterFirst = s.resources.get(test::plastic());

    REQUIRE(l.build(s, "paper_mill"));
    const double secondCost = afterFirst - s.resources.get(test::plastic());
    // Second cost = 50 * 1.15 = 57.5 in a healthy config.
    REQUIRE(secondCost == Approx(57.5));
}

TEST_CASE("Balance: glue_pot cost never becomes free",
          "[balance][regression][!shouldfail]") {
    GameState s;
    content::initNewGame(s);
    s.resources.add(test::plastic(), 10000.0);
    GameLogic l;

    REQUIRE(l.build(s, "glue_pot"));
    REQUIRE(l.build(s, "glue_pot"));
    const double spent = 10000.0 - s.resources.get(test::plastic());
    // 100 + 100*1.15 = 215 in a healthy config.
    REQUIRE(spent == Approx(215.0));
}