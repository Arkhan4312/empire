#include <empire/engine/audio/AudioBusConfig.h>

#include <catch2/catch_test_macros.hpp>
#include <set>
#include <string_view>

using namespace empire::audio;

TEST_CASE("kDefaultBuses: passes compile-time validation", "[audio][bus]") {
    REQUIRE(validateDefaultBuses());
}

TEST_CASE("kDefaultBuses: non-empty", "[audio][bus]") {
    REQUIRE(kDefaultBuses.size() > 0);
}

TEST_CASE("kDefaultBuses: exactly one root (empty parent)", "[audio][bus]") {
    int roots = 0;
    for (const auto& b : kDefaultBuses) {
        if (b.parent.empty()) ++roots;
    }
    REQUIRE(roots == 1);
}

TEST_CASE("kDefaultBuses: master is the root", "[audio][bus]") {
    bool found = false;
    for (const auto& b : kDefaultBuses) {
        if (b.name == "master") {
            REQUIRE(b.parent.empty());
            found = true;
        }
    }
    REQUIRE(found);
}

TEST_CASE("kDefaultBuses: all names unique", "[audio][bus]") {
    std::set<std::string_view> seen;
    for (const auto& b : kDefaultBuses) {
        REQUIRE(seen.insert(b.name).second);
    }
}

TEST_CASE("kDefaultBuses: parents declared before children", "[audio][bus]") {
    std::set<std::string_view> seen;
    for (const auto& b : kDefaultBuses) {
        if (!b.parent.empty()) {
            REQUIRE(seen.count(b.parent) == 1);
        }
        seen.insert(b.name);
    }
}

TEST_CASE("kDefaultBuses: volumes in [0, 1]", "[audio][bus]") {
    for (const auto& b : kDefaultBuses) {
        REQUIRE(b.defaultVolume >= 0.0f);
        REQUIRE(b.defaultVolume <= 1.0f);
    }
}