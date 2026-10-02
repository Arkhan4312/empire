#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "content/Content.h"
#include "core/GameLogic.h"
#include "core/GameState.h"
#include "systems/SaveSystem.h"

using namespace game;
using Catch::Approx;

TEST_CASE("Offline: <60s does nothing", "[offline]") {
    GameState s;
    content::initNewGame(s);
    GameLogic l;
    const double before = s.resources.get(ResourceType::PLASTIC);
    const double sim = SaveSystem::applyOffline(s, l, 30);
    REQUIRE(sim == Approx(0.0));
    REQUIRE(s.resources.get(ResourceType::PLASTIC) == Approx(before));
}

TEST_CASE("Offline: caps at 7 days", "[offline]") {
    GameState s;
    content::initNewGame(s);
    GameLogic l;
    s.resources.rate[(std::size_t)ResourceType::PLASTIC] = 1.0;
    // 30 days
    const auto sim = SaveSystem::applyOffline(s, l, 30LL * 24 * 3600);
    // Cap = 7 days, speed = 0.25 -> ~180000
    REQUIRE(sim <= Approx(180000.0));
}

TEST_CASE("Offline: 1h at full speed", "[offline]") {
    GameState s;
    content::initNewGame(s);
    GameLogic l;
    s.resources.rate[(std::size_t)ResourceType::PLASTIC] = 1.0;
    SaveSystem::applyOffline(s, l, 3600);
    // 1h * speed 1.0 -> 3600 plastic
    REQUIRE(s.resources.get(ResourceType::PLASTIC) ==
            Approx(3600.0).epsilon(0.02));
}

TEST_CASE("Offline: 10h tapers speed", "[offline]") {
    GameState s;
    content::initNewGame(s);
    GameLogic l;
    s.resources.rate[(std::size_t)ResourceType::PLASTIC] = 1.0;
    const double sim = SaveSystem::applyOffline(s, l, 10LL * 3600);
    // Total ~24300
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