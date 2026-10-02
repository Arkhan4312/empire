#include <catch2/catch_test_macros.hpp>

#include "core/GameState.h"

using namespace game;

TEST_CASE("ResourcePool: add accumulates", "[resources]") {
    ResourcePool p;
    p.add(ResourceType::PLASTIC, 10.0);
    p.add(ResourceType::PLASTIC, 2.5);
    REQUIRE(p.get(ResourceType::PLASTIC) == 12.5);
}

TEST_CASE("ResourcePool: spend reduces when affordable", "[resources]") {
    ResourcePool p;
    p.add(ResourceType::PLASTIC, 100.0);
    REQUIRE(p.spend(ResourceType::PLASTIC, 30.0));
    REQUIRE(p.get(ResourceType::PLASTIC) == 70.0);
}

TEST_CASE("ResourcePool: spend fails when insufficient", "[resources]") {
    ResourcePool p;
    p.add(ResourceType::PLASTIC, 5.0);
    REQUIRE_FALSE(p.spend(ResourceType::PLASTIC, 10.0));
    REQUIRE(p.get(ResourceType::PLASTIC) == 5.0);
}

TEST_CASE("ResourcePool: canAfford", "[resources]") {
    ResourcePool p;
    p.add(ResourceType::PAPER, 3.0);
    REQUIRE(p.canAfford(ResourceType::PAPER, 3.0));
    REQUIRE_FALSE(p.canAfford(ResourceType::PAPER, 3.0001));
}

TEST_CASE("ResourcePool: negative add clamps to zero", "[resources]") {
    ResourcePool p;
    p.add(ResourceType::PLASTIC, 5.0);
    p.add(ResourceType::PLASTIC, -100.0);
    REQUIRE(p.get(ResourceType::PLASTIC) == 0.0);
}