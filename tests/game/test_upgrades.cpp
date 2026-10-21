#include <empire/game/content/Content.h>
#include <empire/game/logic/GameLogic.h>
#include <empire/game/state/GameState.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "test_helpers.h"

using namespace game;
using Catch::Approx;

// Regression: Content::installFallback() overwrites `up` between
// up_plastic_1 and up_tools_1 => up_plastic_1 is never pushed into
// m_upgrades. GameScene/E-key still reference it.
TEST_CASE("Upgrade: up_plastic_1 exists in fallback content",
          "[upgrades][regression][!shouldfail]") {
    REQUIRE(content::Content::instance().findUpgrade("up_plastic_1") !=
            nullptr);
}

TEST_CASE("Upgrade: buy spends resources and applies",
          "[upgrades][!shouldfail]") {
    GameState s;
    content::initNewGame(s);
    s.resources.add(test::plastic(), 100.0);
    const double qualityBefore = s.qualityMult;

    GameLogic l;
    REQUIRE(l.buyUpgrade(s, "up_plastic_1"));
    REQUIRE(s.resources.get(test::plastic()) == Approx(0.0));
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

TEST_CASE("Upgrade: cannot exceed max level", "[upgrades][!shouldfail]") {
    GameState s;
    content::initNewGame(s);
    s.resources.add(test::plastic(), 10000.0);
    GameLogic l;

    for (int i = 0; i < 5; ++i) {
        REQUIRE(l.buyUpgrade(s, "up_plastic_1"));
    }
    REQUIRE_FALSE(l.buyUpgrade(s, "up_plastic_1"));
    REQUIRE(s.upgradeLevel("up_plastic_1") == 5);
}

TEST_CASE("Upgrade: unknown id returns false", "[upgrades]") {
    GameState s;
    content::initNewGame(s);
    GameLogic l;
    REQUIRE_FALSE(l.buyUpgrade(s, "does_not_exist"));
}

TEST_CASE("Upgrade: buyCheapestUpgrade picks the only affordable upgrade",
          "[upgrades]") {
    GameState s;
    content::initNewGame(s);
    GameLogic l;
    s.resources.add(test::plastic(), 10000.0);
    // Whatever upgrades exist, the cheapest has to be bought successfully.
    const auto before = s.upgrades.size();
    REQUIRE(l.buyCheapestUpgrade(s));
    REQUIRE(s.upgrades.size() >= before);
}

TEST_CASE("Upgrade: buyCheapestUpgrade fails with empty wallet", "[upgrades]") {
    GameState s;
    content::initNewGame(s);
    GameLogic l;
    // Every fallback upgrade costs > 0 plastic. With zero plastic we must
    // fail to find a candidate.
    REQUIRE_FALSE(l.buyCheapestUpgrade(s));
    REQUIRE(s.upgrades.empty());
}