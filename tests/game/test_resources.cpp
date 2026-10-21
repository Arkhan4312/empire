#include <empire/game/content/Content.h>
#include <empire/game/state/GameState.h>

#include <catch2/catch_test_macros.hpp>

#include "test_helpers.h"

using namespace game;

TEST_CASE("ResourcePool: add accumulates", "[resources]") {
    ResourcePool p;
    test::resetPoolToContent(p);
    p.add(test::plastic(), 10.0);
    p.add(test::plastic(), 2.5);
    REQUIRE(p.get(test::plastic()) == 12.5);
}

TEST_CASE("ResourcePool: spend reduces when affordable", "[resources]") {
    ResourcePool p;
    test::resetPoolToContent(p);
    p.add(test::plastic(), 100.0);
    REQUIRE(p.spend(test::plastic(), 30.0));
    REQUIRE(p.get(test::plastic()) == 70.0);
}

TEST_CASE("ResourcePool: spend fails when insufficient", "[resources]") {
    ResourcePool p;
    test::resetPoolToContent(p);
    p.add(test::plastic(), 5.0);
    REQUIRE_FALSE(p.spend(test::plastic(), 10.0));
    REQUIRE(p.get(test::plastic()) == 5.0);
}

TEST_CASE("ResourcePool: canAfford", "[resources]") {
    ResourcePool p;
    test::resetPoolToContent(p);
    p.add(test::paper(), 3.0);
    REQUIRE(p.canAfford(test::paper(), 3.0));
    REQUIRE_FALSE(p.canAfford(test::paper(), 3.0001));
}

TEST_CASE("ResourcePool: negative add clamps to zero", "[resources]") {
    ResourcePool p;
    test::resetPoolToContent(p);
    p.add(test::plastic(), 5.0);
    p.add(test::plastic(), -100.0);
    REQUIRE(p.get(test::plastic()) == 0.0);
}

TEST_CASE("ResourcePool: setRate / addRate / rateAt", "[resources]") {
    ResourcePool p;
    test::resetPoolToContent(p);
    p.setRate(0, 1.5);
    p.addRate(0, 0.5);
    REQUIRE(p.rateAt(0) == 2.0);
    REQUIRE(p.rateAt(999) == 0.0);
}

TEST_CASE("ResourcePool: zero-sized pool ignores writes", "[resources]") {
    ResourcePool p;  // no reset
    p.add(0, 100.0);
    p.setByIdx(0, 100.0);
    REQUIRE(p.getByIdx(0) == 0.0);
    REQUIRE_FALSE(p.spend(0, 1.0));
}