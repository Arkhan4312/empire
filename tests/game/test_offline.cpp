#include <empire/game/content/Content.h>
#include <empire/game/logic/GameLogic.h>
#include <empire/game/save/SaveSystem.h>
#include <empire/game/state/GameState.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "test_helpers.h"

using namespace game;
using Catch::Approx;

TEST_CASE("Offline: <60s does nothing", "[offline]") {
    GameState s;
    content::initNewGame(s);
    GameLogic l;
    const double before = s.resources.get(test::paper());
    const double sim = SaveSystem::applyOffline(s, l, 30);
    REQUIRE(sim == Approx(0.0));
    REQUIRE(s.resources.get(test::paper()) == Approx(before));
}

TEST_CASE("Offline: caps at 7 days", "[offline]") {
    GameState s;
    content::initNewGame(s);
    s.buildings.push_back({"paper_mill", 1});  // 1 paper/s
    GameLogic l;
    const auto sim = SaveSystem::applyOffline(s, l, 30LL * 24 * 3600);
    // Segments (1.0, 0.75, 0.5, 0.25) applied to 7 days cap => ~179100.
    REQUIRE(sim == Approx(179100.0).epsilon(0.05));
}

TEST_CASE("Offline: 1h at full speed => 1h * rate", "[offline]") {
    GameState s;
    content::initNewGame(s);
    s.buildings.push_back({"paper_mill", 1});
    GameLogic l;
    const double sim = SaveSystem::applyOffline(s, l, 3600);
    REQUIRE(sim == Approx(3600.0).epsilon(0.02));
    // Sub-tick accumulation with dt=1s: ~3600 paper.
    REQUIRE(s.resources.get(test::paper()) == Approx(3600.0).epsilon(0.05));
}

TEST_CASE("Offline: 10h tapers speed", "[offline]") {
    GameState s;
    content::initNewGame(s);
    s.buildings.push_back({"paper_mill", 1});
    GameLogic l;
    const double sim = SaveSystem::applyOffline(s, l, 10LL * 3600);
    // 3600*1.0 + 18000*0.75 + 14400*0.5 = 3600 + 13500 + 7200 = 24300.
    REQUIRE(sim > 20000.0);
    REQUIRE(sim < 26000.0);
}

TEST_CASE("Offline: computeOfflineSeconds", "[offline]") {
    GameState s;
    content::initNewGame(s);
    s.lastSaveTimestamp = 1000;
    REQUIRE(SaveSystem::computeOfflineSeconds(s, 2000) == 1000);
    REQUIRE(SaveSystem::computeOfflineSeconds(s, 500) == 0);
    REQUIRE(SaveSystem::computeOfflineSeconds(s, 1000) == 0);
}

TEST_CASE("Offline: computeOfflineSeconds returns 0 without save",
          "[offline]") {
    GameState s;
    content::initNewGame(s);
    s.lastSaveTimestamp = 0;
    REQUIRE(SaveSystem::computeOfflineSeconds(s, 100000) == 0);
}