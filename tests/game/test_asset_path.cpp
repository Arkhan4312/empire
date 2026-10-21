#include <catch2/catch_test_macros.hpp>

#include <filesystem>

#include <empire/engine/util/AssetPath.h>

TEST_CASE("AssetPath: relative resolves against root", "[asset]") {
    empire::asset::setRoot("/tmp/empire");
    auto p = empire::asset::resolve("sfx/click.wav");
    REQUIRE(p == std::filesystem::path("/tmp/empire/sfx/click.wav"));
}

TEST_CASE("AssetPath: absolute passes through", "[asset]") {
    empire::asset::setRoot("/tmp/empire");
    auto p = empire::asset::resolve("/etc/hosts");
    REQUIRE(p == std::filesystem::path("/etc/hosts"));
}

TEST_CASE("AssetPath: toString matches resolve().string()", "[asset]") {
    empire::asset::setRoot("/tmp/empire");
    REQUIRE(empire::asset::toString("fonts/x.ttf") ==
        empire::asset::resolve("fonts/x.ttf").string());
}

TEST_CASE("AssetPath: root is readable", "[asset]") {
    empire::asset::setRoot("/some/root");
    REQUIRE(empire::asset::root() == std::filesystem::path("/some/root"));
}

TEST_CASE("AssetPath: empty root falls back to relative", "[asset]") {
    empire::asset::setRoot("");
    auto p = empire::asset::resolve("a/b.txt");
    REQUIRE(p == std::filesystem::path("a/b.txt"));
}