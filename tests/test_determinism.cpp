#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "content/Content.h"
#include "core/GameLogic.h"
#include "core/GameState.h"
#include "util/Random.h"

using namespace game;

TEST_CASE("Determinism: identical seed -> identical RNG sequence",
          "[determinism]") {
    Rng a(12345);
    Rng b(12345);
    for (int i = 0; i < 100; ++i) {
        REQUIRE(a.next() == b.next());
    }
}

TEST_CASE("Determinism: identical inputs -> identical state", "[determinism]") {
    auto run = []() {
        GameState s;
        content::initNewGame(s);
        GameLogic l;
        for (int i = 0; i < 10000; ++i) {
            l.clickMain(s);
            if (i % 10 == 0) {
                l.craftUnit(s, "soldier_crooked", 1);
                l.tick(s, TickContext{0.001});
            }
        }
        return s;
    };

    const auto s1 = run();
    const auto s2 = run();

    REQUIRE(s1.resources.get(ResourceType::PLASTIC) ==
            Catch::Approx(s2.resources.get(ResourceType::PLASTIC)));
    REQUIRE(s1.unitCount("soldier_crooked") == s2.unitCount("soldier_crooked"));
    REQUIRE(s1.bossIndex == s2.bossIndex);
    REQUIRE(s1.currentBoss.hp == Catch::Approx(s2.currentBoss.hp));
}