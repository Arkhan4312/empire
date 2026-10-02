#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "content/Content.h"
#include "core/GameLogic.h"
#include "core/GameState.h"

using namespace game;
using Catch::Approx;

TEST_CASE("Upgrade: buy spends resources and applies", "[upgrades]") {
    GameState s;
    content::initNewGame(s);
    s.resources.add(ResourceType::PLASTIC, 100.0);
    const double qualityBefore = s.qualityMult;

    GameLogic l;
    REQUIRE(l.buyUpgrade(s, "up_plastic_1"));
    REQUIRE(s.resources.get(ResourceType::PLASTIC) == Approx(0.0));
    REQUIRE(s.upgradeLevel("up_plastic_1") == 1);
    REQUIRE(s.qualityMult == Approx(qualityBefore * 1.25));
}

TEST_CASE("Upgrade: buy fails without resources", "[upgrades]") {
    GameState s;
    content::initNewGame(s);
    GameLogic l;
    REQUIRE_FALSE(l.buyUpgrade(s, "up_plastic_1"));
    REQUIRE(s.upgradeLevel("up_plastic_1") == 0);
}

TEST_CASE("Upgrade: cannot exceed max level", "[upgrades]") {
    GameState s;
    content::initNewGame(s);
    s.resources.add(ResourceType::PLASTIC, 10000.0);
    GameLogic l;

    REQUIRE(l.buyUpgrade(s, "up_plastic_1"));
    REQUIRE(l.buyUpgrade(s, "up_plastic_1"));
    REQUIRE(l.buyUpgrade(s, "up_plastic_1"));
    REQUIRE(l.buyUpgrade(s, "up_plastic_1"));
    REQUIRE(l.buyUpgrade(s, "up_plastic_1"));
    REQUIRE_FALSE(l.buyUpgrade(s, "up_plastic_1"));
    REQUIRE(s.upgradeLevel("up_plastic_1") == 5);
}

TEST_CASE("Upgrade: unknown id returns false", "[upgrades]") {
    GameState s;
    content::initNewGame(s);
    GameLogic l;
    REQUIRE_FALSE(l.buyUpgrade(s, "does_not_exist"));
}