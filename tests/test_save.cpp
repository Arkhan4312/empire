#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstdio>
#include <filesystem>
#include <string>

#include "content/Content.h"
#include "core/GameLogic.h"
#include "core/GameState.h"
#include "systems/SaveSystem.h"
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
        }
    }
    TempFile(const TempFile&) = delete;
    TempFile& operator=(const TempFile&) = delete;
};

TEST_CASE("Save: round-trip preserves state", "[save]") {
    GameState s;
    content::initNewGame(s);
    s.resources.add(ResourceType::PLASTIC, 123.0);
    s.resources.add(ResourceType::PAPER, 42.0);
    s.units.push_back({"soldier_crooked", 7});
    s.units.push_back({"tank_matchbox", 2});
    s.upgrades.push_back({"up_plastic_1", 2});
    s.clickPower = 1.5;
    s.qualityMult = 1.25;
    s.speedMult = 1.1;
    s.bossIndex = 1;
    s.playTimeSeconds = 3600;
    s.rngSeed = 42;

    const auto path = TempFile("empire_roundtrip.json");
    SaveSystem sys;
    REQUIRE(sys.save(s, path.path));

    GameState save;
    content::initNewGame(save);
    REQUIRE(sys.load(save, path.path));

    REQUIRE(save.resources.get(ResourceType::PLASTIC) == Approx(123.0));
    REQUIRE(save.resources.get(ResourceType::PAPER) == Approx(42.0));
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
    const auto path = TempFile("empire_corrupt.json");
    {
        std::FILE* f = std::fopen(path.path.c_str(), "w");
        REQUIRE(f != nullptr);
        std::fputs("not a json {{{{{{", f);
        std::fclose(f);
    }
    GameState s;
    content::initNewGame(s);
    SaveSystem sys;
    REQUIRE_FALSE(sys.load(s, path.path));
}