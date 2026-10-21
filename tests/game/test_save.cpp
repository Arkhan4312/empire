#include <empire/game/content/Content.h>
#include <empire/game/save/SaveSystem.h>
#include <empire/game/state/GameState.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

#include "test_helpers.h"

using namespace game;
using Catch::Approx;

struct TempFile {
    std::string path;
    explicit TempFile(const std::string& name) {
        path = (std::filesystem::temp_directory_path() / name).string();
    }
    ~TempFile() {
        if (!path.empty()) {
            std::remove(path.c_str());
            std::remove((path + ".tmp").c_str());
            std::remove((path + ".bak").c_str());
        }
    }
    TempFile(const TempFile&) = delete;
    TempFile& operator=(const TempFile&) = delete;
};

TEST_CASE("Save: round-trip preserves state", "[save]") {
    GameState s;
    content::initNewGame(s);
    s.resources.add(test::plastic(), 123.0);
    s.resources.add(test::paper(), 42.0);
    s.units.push_back({"soldier_crooked", 7});
    s.units.push_back({"tank_matchbox", 2});
    s.upgrades.push_back({"up_plastic_1", 2});
    s.clickPower = 1.5;
    s.qualityMult = 1.25;
    s.speedMult = 1.1;
    s.bossIndex = 1;
    s.playTimeSeconds = 3600;
    s.rngSeed = 42;

    const auto tf = TempFile("empire_roundtrip.json");
    SaveSystem sys;
    REQUIRE(sys.save(s, tf.path));

    GameState save;
    content::initNewGame(save);
    REQUIRE(sys.load(save, tf.path));

    REQUIRE(save.resources.get(test::plastic()) == Approx(123.0));
    REQUIRE(save.resources.get(test::paper()) == Approx(42.0));
    REQUIRE(save.unitCount("soldier_crooked") == 7);
    REQUIRE(save.unitCount("tank_matchbox") == 2);
    REQUIRE(save.upgradeLevel("up_plastic_1") == 2);
    REQUIRE(save.clickPower == Approx(1.5));
    REQUIRE(save.qualityMult == Approx(1.25));
    REQUIRE(save.speedMult == Approx(1.1));
    REQUIRE(save.bossIndex == 1);
    REQUIRE(save.playTimeSeconds == 3600);
    REQUIRE(save.rngSeed == 42u);
}

TEST_CASE("Save: load missing file returns false", "[save]") {
    GameState s;
    content::initNewGame(s);
    SaveSystem sys;
    REQUIRE_FALSE(sys.load(s, "/tmp/_no_such_file_.json"));
}

TEST_CASE("Save: corrupt JSON returns false without crashing", "[save]") {
    const auto tf = TempFile("empire_corrupt.json");
    {
        std::ofstream f(tf.path);
        REQUIRE(f.good());
        f << "not a json {{{{{{";
    }
    GameState s;
    content::initNewGame(s);
    SaveSystem sys;
    REQUIRE_FALSE(sys.load(s, tf.path));
}

TEST_CASE("Save: future version is rejected", "[save]") {
    const auto tf = TempFile("empire_future.json");
    {
        std::ofstream f(tf.path);
        f << R"({"version": 9999, "era": 1})";
    }
    GameState s;
    content::initNewGame(s);
    SaveSystem sys;
    REQUIRE_FALSE(sys.load(s, tf.path));
}

TEST_CASE("Save: sanitizer drops unknown unit ids", "[save]") {
    const auto tf = TempFile("empire_unknown_unit.json");
    {
        std::ofstream f(tf.path);
        f << R"({
            "version": 1, "era": 1,
            "units": [{"id":"alien_dreadnought","count":5}],
            "upgrades": [{"id":"ghost_upgrade","level":1}]
        })";
    }
    GameState s;
    content::initNewGame(s);
    SaveSystem sys;
    REQUIRE(sys.load(s, tf.path));
    REQUIRE(s.unitCount("alien_dreadnought") == 0);
    REQUIRE(s.upgradeLevel("ghost_upgrade") == 0);
}

TEST_CASE("Save: empty save file fails cleanly", "[save]") {
    const auto tf = TempFile("empire_empty.json");
    {
        std::ofstream f(tf.path);
    }  // touch
    GameState s;
    content::initNewGame(s);
    SaveSystem sys;
    REQUIRE_FALSE(sys.load(s, tf.path));
}

TEST_CASE("Save: sanitizer clamps negative clickPower", "[save]") {
    const auto tf = TempFile("empire_negclick.json");
    {
        std::ofstream f(tf.path);
        f << R"({"version":1,"era":1,"clickPower":-5.0})";
    }
    GameState s;
    content::initNewGame(s);
    SaveSystem sys;
    REQUIRE(sys.load(s, tf.path));
    REQUIRE(s.clickPower >= 1.0);
}