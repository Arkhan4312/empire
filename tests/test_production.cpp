#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "content/Content.h"
#include "core/GameLogic.h"
#include "core/GameState.h"

using namespace game;
using Catch::Approx;

TEST_CASE("Production: click adds plastic", "[production]") {
    GameState s;
    content::initNewGame(s);
    GameLogic l;
    l.clickMain(s);
    REQUIRE(s.resources.get(ResourceType::PLASTIC) == Approx(1.0));
}

TEST_CASE("Production: clickPower depends on quality", "[production]") {
    GameState s;
    content::initNewGame(s);
    s.qualityMult = 2.0;
    GameLogic l;
    l.clickMain(s);
    REQUIRE(s.resources.get(ResourceType::PLASTIC) == Approx(2.0));
}

TEST_CASE("Production: tick applies rate", "[production]") {
    GameState s;
    content::initNewGame(s);
    s.resources.rate[(std::size_t)ResourceType::PLASTIC] = 1.0;
    GameLogic l;
    TickContext ctx{10.0};
    l.tick(s, ctx);
    REQUIRE(s.resources.get(ResourceType::PLASTIC) == Approx(10.0));
}