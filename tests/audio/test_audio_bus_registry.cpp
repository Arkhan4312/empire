#include <empire/engine/audio/AudioBus.h>
#include <empire/engine/audio/AudioBusRegistry.h>

#include <algorithm>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <string_view>

using namespace empire::audio;
using Catch::Approx;

// -------------------------------------------------------------------------
// Local helpers
// -------------------------------------------------------------------------
namespace {

float currentVolumeOf(AudioBusRegistry& buses, std::string_view name) {
    float v = -1.0f;
    buses.forEach([&](BusHandle, const AudioBus& b) {
        if (b.name == name) v = b.currentVolume;
    });
    return v;
}

bool soloKillOf(AudioBusRegistry& buses, std::string_view name) {
    bool k = false;
    buses.forEach([&](BusHandle, const AudioBus& b) {
        if (b.name == name) k = b.soloKill;
    });
    return k;
}

}  // namespace

// =========================================================================
// ensure / find / contains / size
// =========================================================================

TEST_CASE("AudioBusRegistry: starts empty", "[audio][bus]") {
    AudioBusRegistry buses;
    REQUIRE(buses.size() == 0u);
    REQUIRE_FALSE(buses.contains("master"));
    REQUIRE_FALSE(buses.find("master").isValid());
}

TEST_CASE("AudioBusRegistry: ensure creates a root bus", "[audio][bus]") {
    AudioBusRegistry buses;
    auto r = buses.ensure("master");
    REQUIRE(r.isOk());
    REQUIRE(r.value().isValid());
    REQUIRE(buses.size() == 1u);
    REQUIRE(buses.contains("master"));
    REQUIRE(buses.find("master") == r.value());
}

TEST_CASE("AudioBusRegistry: ensure a child requires existing parent",
          "[audio][bus]") {
    AudioBusRegistry buses;
    // No parent → BusNotFound
    auto r = buses.ensure("music", "master");
    REQUIRE(r.isErr());
    REQUIRE(r.error() == AudioError::BusNotFound);

    REQUIRE(buses.ensure("master").isOk());
    auto r2 = buses.ensure("music", "master");
    REQUIRE(r2.isOk());
    REQUIRE(buses.size() == 2u);
}

TEST_CASE("AudioBusRegistry: ensure is idempotent for same parent",
          "[audio][bus]") {
    AudioBusRegistry buses;
    buses.ensure("master");
    const auto h1 = buses.ensure("music", "master").value();
    const auto h2 = buses.ensure("music", "master").value();
    REQUIRE(h1 == h2);
    REQUIRE(buses.size() == 2u);
}

TEST_CASE("AudioBusRegistry: ensure with different parent -> BusAlreadyExists",
          "[audio][bus]") {
    AudioBusRegistry buses;
    buses.ensure("master");
    buses.ensure("sfx", "master");
    buses.ensure("music", "master");

    auto r = buses.ensure("music", "sfx");
    REQUIRE(r.isErr());
    REQUIRE(r.error() == AudioError::BusAlreadyExists);
}

TEST_CASE("AudioBusRegistry: ensure with empty name -> InvalidArgument",
          "[audio][bus]") {
    AudioBusRegistry buses;
    auto r = buses.ensure("");
    REQUIRE(r.isErr());
    REQUIRE(r.error() == AudioError::InvalidArgument);
}

TEST_CASE("AudioBusRegistry: ensure links child into parent's list",
          "[audio][bus]") {
    AudioBusRegistry buses;
    const auto master = buses.ensure("master").value();
    const auto music = buses.ensure("music", "master").value();

    bool found = false;
    buses.forEach([&](BusHandle h, const AudioBus& b) {
        if (h == master) {
            found = std::find(b.children.begin(), b.children.end(), music) !=
                    b.children.end();
        }
    });
    REQUIRE(found);
}
// =========================================================================
// initDefaults
// =========================================================================

TEST_CASE("AudioBusRegistry: initDefaults creates all default buses",
          "[audio][bus][defaults]") {
    AudioBusRegistry buses;
    REQUIRE(buses.initDefaults().isOk());
    REQUIRE(buses.size() == 6u);
    REQUIRE(buses.contains("master"));
    REQUIRE(buses.contains("music"));
    REQUIRE(buses.contains("sfx"));
    REQUIRE(buses.contains("ui"));
    REQUIRE(buses.contains("voice"));
    REQUIRE(buses.contains("ambient"));
}

TEST_CASE("AudioBusRegistry: initDefaults applies default volumes",
          "[audio][bus][defaults]") {
    AudioBusRegistry buses;
    REQUIRE(buses.initDefaults().isOk());
    REQUIRE(buses.volume(buses.find("master")) == Approx(1.0f));
    REQUIRE(buses.volume(buses.find("music")) == Approx(0.6f));
    REQUIRE(buses.volume(buses.find("sfx")) == Approx(1.0f));
    REQUIRE(buses.volume(buses.find("ui")) == Approx(0.9f));
    REQUIRE(buses.volume(buses.find("voice")) == Approx(1.0f));
    REQUIRE(buses.volume(buses.find("ambient")) == Approx(0.5f));
}

TEST_CASE("AudioBusRegistry: initDefaults is idempotent",
          "[audio][bus][defaults]") {
    AudioBusRegistry buses;
    REQUIRE(buses.initDefaults().isOk());
    const float before = buses.volume(buses.find("music"));
    REQUIRE(buses.initDefaults().isOk());
    REQUIRE(buses.volume(buses.find("music")) == Approx(before));
    REQUIRE(buses.size() == 6u);
}

TEST_CASE("AudioBusRegistry: user-set volume survives initDefaults",
          "[audio][bus][defaults]") {
    AudioBusRegistry buses;
    buses.ensure("master");
    buses.ensure("music", "master");
    buses.setVolume(buses.find("music"), 0.25f);

    REQUIRE(buses.initDefaults().isOk());
    REQUIRE(buses.volume(buses.find("music")) == Approx(0.25f));
}

// =========================================================================
// setVolume / volume
// =========================================================================

TEST_CASE("AudioBusRegistry: setVolume clamps to [0, 1]", "[audio][bus][vol]") {
    AudioBusRegistry buses;
    const auto h = buses.ensure("master").value();

    buses.setVolume(h, -0.5f);
    REQUIRE(buses.volume(h) == Approx(0.0f));

    buses.setVolume(h, 2.0f);
    REQUIRE(buses.volume(h) == Approx(1.0f));

    buses.setVolume(h, 0.3f);
    REQUIRE(buses.volume(h) == Approx(0.3f));
}

TEST_CASE("AudioBusRegistry: setVolume also updates currentVolume",
          "[audio][bus][vol]") {
    AudioBusRegistry buses;
    const auto h = buses.ensure("master").value();
    buses.setVolume(h, 0.4f);
    REQUIRE(currentVolumeOf(buses, "master") == Approx(0.4f));
}

TEST_CASE("AudioBusRegistry: setVolume on invalid handle is a no-op",
          "[audio][bus][vol]") {
    AudioBusRegistry buses;
    REQUIRE_NOTHROW(buses.setVolume(kInvalidBus, 0.5f));
}

TEST_CASE("AudioBusRegistry: volume of invalid handle returns 0",
          "[audio][bus][vol]") {
    AudioBusRegistry buses;
    REQUIRE(buses.volume(kInvalidBus) == 0.0f);
}

// =========================================================================
// setMuted / muted
// =========================================================================

TEST_CASE("AudioBusRegistry: mute toggles state", "[audio][bus][mute]") {
    AudioBusRegistry buses;
    const auto h = buses.ensure("master").value();

    REQUIRE_FALSE(buses.muted(h));
    buses.setMuted(h, true);
    REQUIRE(buses.muted(h));
    buses.setMuted(h, false);
    REQUIRE_FALSE(buses.muted(h));
}

TEST_CASE("AudioBusRegistry: muted on invalid handle returns false",
          "[audio][bus][mute]") {
    AudioBusRegistry buses;
    REQUIRE_FALSE(buses.muted(kInvalidBus));
}

// =========================================================================
// setSolo / anySolo / clearAllSolo
// =========================================================================

TEST_CASE("AudioBusRegistry: initial solo state", "[audio][bus][solo]") {
    AudioBusRegistry buses;
    REQUIRE(buses.initDefaults().isOk());
    REQUIRE_FALSE(buses.anySolo());
    REQUIRE_FALSE(soloKillOf(buses, "music"));
    REQUIRE_FALSE(soloKillOf(buses, "sfx"));
}

TEST_CASE("AudioBusRegistry: solo a leaf kills other leaves only",
          "[audio][bus][solo]") {
    AudioBusRegistry buses;
    REQUIRE(buses.initDefaults().isOk());
    buses.setSolo(buses.find("music"), true);

    REQUIRE(buses.anySolo());
    REQUIRE_FALSE(soloKillOf(buses, "music"));
    REQUIRE(soloKillOf(buses, "sfx"));
    REQUIRE(soloKillOf(buses, "ui"));
    REQUIRE(soloKillOf(buses, "voice"));
    REQUIRE(soloKillOf(buses, "ambient"));
    // Master is an ancestor of the solo'd bus → not killed
    REQUIRE_FALSE(soloKillOf(buses, "master"));
}

TEST_CASE("AudioBusRegistry: solo two leaves kills only the rest",
          "[audio][bus][solo]") {
    AudioBusRegistry buses;
    REQUIRE(buses.initDefaults().isOk());
    buses.setSolo(buses.find("music"), true);
    buses.setSolo(buses.find("sfx"), true);

    REQUIRE(buses.anySolo());
    REQUIRE_FALSE(soloKillOf(buses, "music"));
    REQUIRE_FALSE(soloKillOf(buses, "sfx"));
    REQUIRE(soloKillOf(buses, "ui"));
    REQUIRE(soloKillOf(buses, "voice"));
    REQUIRE(soloKillOf(buses, "ambient"));
}

TEST_CASE("AudioBusRegistry: solo root currently kills its children",
          "[audio][bus][solo][bug]") {
    // NOTE: current implementation of recomputeSoloState() kills descendants
    // when the root itself is solo'd — soloing "master" silences everything.
    // This test documents the actual behaviour; flagged for review.
    AudioBusRegistry buses;
    REQUIRE(buses.initDefaults().isOk());
    buses.setSolo(buses.find("master"), true);

    REQUIRE(buses.anySolo());
    REQUIRE_FALSE(soloKillOf(buses, "master"));
    // Current (possibly unintended) behaviour:
    REQUIRE(soloKillOf(buses, "music"));
    REQUIRE(soloKillOf(buses, "sfx"));
}

TEST_CASE("AudioBusRegistry: setSolo no-op when value unchanged",
          "[audio][bus][solo]") {
    AudioBusRegistry buses;
    REQUIRE(buses.initDefaults().isOk());
    // Setting solo(false) when already false must not change any state.
    buses.setSolo(buses.find("music"), false);
    REQUIRE_FALSE(buses.anySolo());
    REQUIRE_FALSE(soloKillOf(buses, "sfx"));
}

TEST_CASE("AudioBusRegistry: clearAllSolo resets everything",
          "[audio][bus][solo]") {
    AudioBusRegistry buses;
    REQUIRE(buses.initDefaults().isOk());
    buses.setSolo(buses.find("music"), true);
    buses.setSolo(buses.find("sfx"), true);
    REQUIRE(buses.anySolo());

    buses.clearAllSolo();
    REQUIRE_FALSE(buses.anySolo());
    REQUIRE_FALSE(soloKillOf(buses, "music"));
    REQUIRE_FALSE(soloKillOf(buses, "sfx"));
    REQUIRE_FALSE(soloKillOf(buses, "ui"));
    REQUIRE_FALSE(soloKillOf(buses, "voice"));
    REQUIRE_FALSE(soloKillOf(buses, "ambient"));
}

TEST_CASE("AudioBusRegistry: removing a solo'd bus recomputes solo state",
          "[audio][bus][solo][remove]") {
    AudioBusRegistry buses;
    REQUIRE(buses.initDefaults().isOk());
    buses.setSolo(buses.find("music"), true);

    REQUIRE(buses.remove(buses.find("music")).isOk());
    REQUIRE_FALSE(buses.anySolo());
    REQUIRE_FALSE(soloKillOf(buses, "sfx"));
}

// =========================================================================
// applySnapshot / update (fade state machine)
// =========================================================================

TEST_CASE("AudioBusRegistry: applySnapshot with fadeSec=0 is immediate",
          "[audio][bus][snapshot]") {
    AudioBusRegistry buses;
    REQUIRE(buses.initDefaults().isOk());

    AudioBusSnapshot snap{{{"music", 0.3f}}};
    buses.applySnapshot(snap, 0.0f);

    REQUIRE(buses.volume(buses.find("music")) == Approx(0.3f));
    REQUIRE(currentVolumeOf(buses, "music") == Approx(0.3f));
}

TEST_CASE("AudioBusRegistry: applySnapshot clamps target",
          "[audio][bus][snapshot]") {
    AudioBusRegistry buses;
    REQUIRE(buses.initDefaults().isOk());
    AudioBusSnapshot snap{{{"music", 5.0f}, {"sfx", -1.0f}}};
    buses.applySnapshot(snap, 0.0f);
    REQUIRE(buses.volume(buses.find("music")) == Approx(1.0f));
    REQUIRE(buses.volume(buses.find("sfx")) == Approx(0.0f));
}

TEST_CASE("AudioBusRegistry: applySnapshot ignores unknown buses",
          "[audio][bus][snapshot]") {
    AudioBusRegistry buses;
    REQUIRE(buses.initDefaults().isOk());
    AudioBusSnapshot snap{{{"music", 0.3f}, {"nonexistent", 0.5f}}};
    REQUIRE_NOTHROW(buses.applySnapshot(snap, 0.0f));
    REQUIRE(buses.volume(buses.find("music")) == Approx(0.3f));
}

TEST_CASE("AudioBusRegistry: applySnapshot with fade defers current volume",
          "[audio][bus][snapshot][fade]") {
    AudioBusRegistry buses;
    REQUIRE(buses.initDefaults().isOk());
    // Music default = 0.6
    AudioBusSnapshot snap{{{"music", 0.0f}}};
    buses.applySnapshot(snap, 1.0f);

    REQUIRE(buses.volume(buses.find("music")) == Approx(0.0f));
    REQUIRE(currentVolumeOf(buses, "music") == Approx(0.6f));
}

TEST_CASE("AudioBusRegistry: update interpolates linearly across fade",
          "[audio][bus][fade]") {
    AudioBusRegistry buses;
    REQUIRE(buses.initDefaults().isOk());
    // Use sfx=1.0 → 0.0, fade=1.0s
    buses.setVolume(buses.find("sfx"), 1.0f);
    AudioBusSnapshot snap{{{"sfx", 0.0f}}};
    buses.applySnapshot(snap, 1.0f);

    buses.update(0.25f);
    REQUIRE(currentVolumeOf(buses, "sfx") == Approx(0.75f));

    buses.update(0.25f);
    REQUIRE(currentVolumeOf(buses, "sfx") == Approx(0.5f));

    buses.update(0.25f);
    REQUIRE(currentVolumeOf(buses, "sfx") == Approx(0.25f));

    buses.update(0.25f);
    REQUIRE(currentVolumeOf(buses, "sfx") == Approx(0.0f));
}

TEST_CASE("AudioBusRegistry: update beyond fade duration snaps to target",
          "[audio][bus][fade]") {
    AudioBusRegistry buses;
    REQUIRE(buses.initDefaults().isOk());
    buses.setVolume(buses.find("sfx"), 1.0f);

    buses.applySnapshot({{{"sfx", 0.0f}}}, 1.0f);
    buses.update(10.0f);  // way past the fade
    REQUIRE(currentVolumeOf(buses, "sfx") == Approx(0.0f));
    REQUIRE(buses.volume(buses.find("sfx")) == Approx(0.0f));
}

TEST_CASE("AudioBusRegistry: update with dt<=0 is a no-op",
          "[audio][bus][fade]") {
    AudioBusRegistry buses;
    REQUIRE(buses.initDefaults().isOk());
    buses.setVolume(buses.find("sfx"), 1.0f);
    buses.applySnapshot({{{"sfx", 0.0f}}}, 1.0f);

    buses.update(0.0f);
    REQUIRE(currentVolumeOf(buses, "sfx") == Approx(1.0f));

    buses.update(-0.5f);
    REQUIRE(currentVolumeOf(buses, "sfx") == Approx(1.0f));
}

TEST_CASE("AudioBusRegistry: two buses can fade independently",
          "[audio][bus][fade]") {
    AudioBusRegistry buses;
    REQUIRE(buses.initDefaults().isOk());
    buses.setVolume(buses.find("sfx"), 1.0f);
    buses.setVolume(buses.find("ui"), 1.0f);

    buses.applySnapshot({{{"sfx", 0.0f}}}, 1.0f);
    buses.applySnapshot({{{"ui", 0.0f}}}, 2.0f);

    buses.update(0.5f);
    REQUIRE(currentVolumeOf(buses, "sfx") == Approx(0.5f));
    REQUIRE(currentVolumeOf(buses, "ui") == Approx(0.75f));

    buses.update(0.5f);
    REQUIRE(currentVolumeOf(buses, "sfx") == Approx(0.0f));
    REQUIRE(currentVolumeOf(buses, "ui") == Approx(0.5f));

    buses.update(1.0f);
    REQUIRE(currentVolumeOf(buses, "sfx") == Approx(0.0f));
    REQUIRE(currentVolumeOf(buses, "ui") == Approx(0.0f));
}

TEST_CASE("AudioBusRegistry: setVolume cancels in-flight fade",
          "[audio][bus][fade]") {
    AudioBusRegistry buses;
    REQUIRE(buses.initDefaults().isOk());
    buses.setVolume(buses.find("sfx"), 1.0f);
    buses.applySnapshot({{{"sfx", 0.0f}}}, 1.0f);
    buses.update(0.5f);  // halfway
    REQUIRE(currentVolumeOf(buses, "sfx") == Approx(0.5f));

    buses.setVolume(buses.find("sfx"), 0.2f);
    REQUIRE(buses.volume(buses.find("sfx")) == Approx(0.2f));
    REQUIRE(currentVolumeOf(buses, "sfx") == Approx(0.2f));

    // Update should be a no-op since fade was cancelled
    buses.update(0.5f);
    REQUIRE(currentVolumeOf(buses, "sfx") == Approx(0.2f));
}

TEST_CASE("AudioBusRegistry: re-applySnapshot mid-fade re-starts from current",
          "[audio][bus][fade]") {
    AudioBusRegistry buses;
    REQUIRE(buses.initDefaults().isOk());
    buses.setVolume(buses.find("sfx"), 1.0f);
    buses.applySnapshot({{{"sfx", 0.0f}}}, 1.0f);
    buses.update(0.5f);
    REQUIRE(currentVolumeOf(buses, "sfx") == Approx(0.5f));

    // New target from current=0.5
    buses.applySnapshot({{{"sfx", 1.0f}}}, 1.0f);
    REQUIRE(buses.volume(buses.find("sfx")) == Approx(1.0f));
    REQUIRE(currentVolumeOf(buses, "sfx") == Approx(0.5f));

    buses.update(0.5f);
    // Linear from 0.5 to 1.0, halfway → 0.75
    REQUIRE(currentVolumeOf(buses, "sfx") == Approx(0.75f));
}

// =========================================================================
// remove
// =========================================================================

TEST_CASE("AudioBusRegistry: remove a leaf", "[audio][bus][remove]") {
    AudioBusRegistry buses;
    REQUIRE(buses.initDefaults().isOk());
    const auto h = buses.find("music");
    REQUIRE(buses.remove(h).isOk());
    REQUIRE(buses.size() == 5u);
    REQUIRE_FALSE(buses.contains("music"));
    REQUIRE_FALSE(buses.find("music").isValid());
}

TEST_CASE("AudioBusRegistry: remove a parent with children -> InvalidArgument",
          "[audio][bus][remove]") {
    AudioBusRegistry buses;
    REQUIRE(buses.initDefaults().isOk());
    auto r = buses.remove(buses.find("master"));
    REQUIRE(r.isErr());
    REQUIRE(r.error() == AudioError::InvalidArgument);
}

TEST_CASE("AudioBusRegistry: remove invalid handle -> BusNotFound",
          "[audio][bus][remove]") {
    AudioBusRegistry buses;
    auto r = buses.remove(kInvalidBus);
    REQUIRE(r.isErr());
    REQUIRE(r.error() == AudioError::BusNotFound);
}

TEST_CASE("AudioBusRegistry: remove unlinks from parent's children",
          "[audio][bus][remove]") {
    AudioBusRegistry buses;
    REQUIRE(buses.initDefaults().isOk());
    const auto music = buses.find("music");
    const auto master = buses.find("master");

    REQUIRE(buses.remove(music).isOk());

    bool stillListed = true;
    buses.forEach([&](BusHandle h, const AudioBus& b) {
        if (h == master) {
            stillListed = std::find(b.children.begin(), b.children.end(),
                                    music) != b.children.end();
        }
    });
    REQUIRE_FALSE(stillListed);
}

// =========================================================================
// forEach
// =========================================================================

TEST_CASE("AudioBusRegistry: forEach visits every bus exactly once",
          "[audio][bus]") {
    AudioBusRegistry buses;
    REQUIRE(buses.initDefaults().isOk());

    int count = 0;
    buses.forEach([&](BusHandle, const AudioBus&) { ++count; });
    REQUIRE(count == 6);
}

TEST_CASE("AudioBusRegistry: forEach with mutable lambda can modify bus",
          "[audio][bus]") {
    AudioBusRegistry buses;
    REQUIRE(buses.initDefaults().isOk());
    buses.forEach([](BusHandle, AudioBus& b) { b.volume = 0.5f; });
    REQUIRE(buses.volume(buses.find("music")) == Approx(0.5f));
}