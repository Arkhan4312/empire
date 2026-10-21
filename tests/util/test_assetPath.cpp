#include <empire/engine/util/AssetPath.h>

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <string>
#include <string_view>

namespace fs = std::filesystem;
namespace asset = empire::asset;

namespace {
struct ScopedRoot {
    explicit ScopedRoot(fs::path r = {}) : m_prev(asset::root()) {
        asset::setRoot(std::move(r));
    }
    ~ScopedRoot() {
        asset::setRoot(std::move(m_prev));
    }
    ScopedRoot(const ScopedRoot&) = delete;
    ScopedRoot& operator=(const ScopedRoot&) = delete;
    ScopedRoot(ScopedRoot&&) = delete;
    ScopedRoot& operator=(ScopedRoot&&) = delete;

private:
    fs::path m_prev;
};
}  // namespace

TEST_CASE("AssetPath: default root is empty", "[util][asset]") {
    ScopedRoot root{};          // явно сбрасываем
    REQUIRE(asset::root().empty());
}

TEST_CASE("AssetPath: setRoot then root round-trips", "[util][asset]") {
    ScopedRoot root{};
    asset::setRoot(fs::path{ "/some/base" });
    REQUIRE(asset::root() == fs::path{ "/some/base" });
}

TEST_CASE("AssetPath: resolve relative path joins root", "[util][asset]") {
    ScopedRoot root{ fs::path{"/base"} };
    REQUIRE(asset::resolve("sound.wav") == fs::path{ "/base/sound.wav" });
    REQUIRE(asset::resolve("sub/dir/sound.wav") ==
        fs::path{ "/base/sub/dir/sound.wav" });
}

TEST_CASE("AssetPath: resolve absolute path passes through",
    "[util][asset]") {
    ScopedRoot root{ fs::path{"/base"} };
    const fs::path abs = fs::absolute(fs::temp_directory_path() / "abs.wav");
    REQUIRE(asset::resolve(abs.string()) == abs);
}

TEST_CASE("AssetPath: toString returns native string of resolved path",
    "[util][asset]") {
    ScopedRoot root{ fs::path{"/base"} };
    const std::string got = asset::toString("file.wav");
    const std::string want = (fs::path{ "/base" } / "file.wav").string();
    REQUIRE(got == want);
}