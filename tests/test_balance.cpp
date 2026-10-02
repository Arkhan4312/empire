#include <catch2/catch_test_macros.hpp>

#include "content/Content.h"
#include "core/GameLogic.h"
#include "core/GameState.h"

using namespace game;

// simple bot: click when can't afford new soldier, craft if he can;
static void botStep(GameState& s, GameLogic& l) {
    if (s.resources.get(ResourceType::PLASTIC) >= 5.0) {
        l.craftUnit(s, "soldier_crooked", 1);
    } else {
        l.clickMain(s);
    }
}

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
        if (s.era == Era::ENTREPRENEUR) break;
    }
    INFO("Ticks taken: " << tick);
    INFO("Boss index: " << s.bossIndex);
    INFO("Era: " << (int)s.era);
    REQUIRE(s.era == Era::ENTREPRENEUR);
}

TEST_CASE("Balance: bot never causes negative resources", "[balance]") {
    GameState s;
    content::initNewGame(s);
    GameLogic l;

    for (int i = 0; i < 100000; ++i) {
        botStep(s, l);
        l.tick(s, TickContext{0.001});
        for (std::size_t r = 0; r < kResourceCount; ++r) {
            REQUIRE(s.resources.amount[r] >= 0.0);
        }
    }
}