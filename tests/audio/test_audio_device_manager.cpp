#include <empire/engine/audio/AudioDeviceManager.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace empire::audio;
using Catch::Approx;

TEST_CASE("AudioDeviceManager: pre-init defaults", "[audio][device][preinit]") {
    AudioDeviceManager dev;
    REQUIRE_FALSE(dev.ready());
    REQUIRE_FALSE(dev.isMigrating());
    REQUIRE(dev.currentDevice() == -1);
    REQUIRE(dev.migrationGain() == Approx(1.0f));
    REQUIRE(dev.listOutputDevices().empty());
}

TEST_CASE("AudioDeviceManager: setPollInterval clamps to [0.1, 10]",
          "[audio][device][config]") {
    AudioDeviceManager dev;

    dev.setPollInterval(0.001f);
    REQUIRE(dev.pollInterval() == Approx(0.1f));

    dev.setPollInterval(0.5f);
    REQUIRE(dev.pollInterval() == Approx(0.5f));

    dev.setPollInterval(1000.0f);
    REQUIRE(dev.pollInterval() == Approx(10.0f));
}

TEST_CASE("AudioDeviceManager: setMigrationFadeDurations clamps negatives",
          "[audio][device][config]") {
    AudioDeviceManager dev;
    dev.setMigrationFadeDurations(-1.0f, -1.0f);
    REQUIRE(dev.migrationFadeOut() == Approx(0.0f));
    REQUIRE(dev.migrationFadeIn() == Approx(0.0f));

    dev.setMigrationFadeDurations(0.5f, 0.3f);
    REQUIRE(dev.migrationFadeOut() == Approx(0.5f));
    REQUIRE(dev.migrationFadeIn() == Approx(0.3f));
}

TEST_CASE("AudioDeviceManager: setRetryBackoff clamps and initialises",
          "[audio][device][config]") {
    AudioDeviceManager dev;

    // Too low initial → clamped to 0.05
    dev.setRetryBackoff(0.001f, 10.0f);
    REQUIRE(dev.retryBackoffCurrent() == Approx(0.05f).margin(1e-6));

    // Valid values pass through
    dev.setRetryBackoff(0.5f, 10.0f);
    REQUIRE(dev.retryBackoffCurrent() == Approx(0.5f));

    // max < initial → clamped up to initial
    dev.setRetryBackoff(5.0f, 1.0f);
    REQUIRE(dev.retryBackoffCurrent() == Approx(5.0f));
}

TEST_CASE("AudioDeviceManager: autoFollowDefault getter/setter",
          "[audio][device][config]") {
    AudioDeviceManager dev;
    REQUIRE(dev.autoFollowDefault());
    dev.setAutoFollowDefault(false);
    REQUIRE_FALSE(dev.autoFollowDefault());
    dev.setAutoFollowDefault(true);
    REQUIRE(dev.autoFollowDefault());
}

TEST_CASE("AudioDeviceManager: setOutputDevice pre-init -> NotInitialized",
          "[audio][device][preinit]") {
    AudioDeviceManager dev;
    auto r0 = dev.setOutputDevice(0);
    REQUIRE(r0.isErr());
    REQUIRE(r0.error() == AudioError::NotInitialized);

    auto rDefault = dev.setOutputDevice(-1);
    REQUIRE(rDefault.isErr());
    REQUIRE(rDefault.error() == AudioError::NotInitialized);
}

TEST_CASE("AudioDeviceManager: shutdown before init is safe",
          "[audio][device][preinit]") {
    AudioDeviceManager dev;
    REQUIRE_NOTHROW(dev.shutdown());
    REQUIRE_FALSE(dev.ready());
}

TEST_CASE("AudioDeviceManager: update before init is safe",
          "[audio][device][preinit]") {
    AudioDeviceManager dev;
    REQUIRE_NOTHROW(dev.update(0.016f));
    REQUIRE_NOTHROW(dev.update(1.0f));
}

TEST_CASE("AudioDeviceManager: followsSystemDefault default is true",
          "[audio][device][preinit]") {
    AudioDeviceManager dev;
    REQUIRE(dev.followsSystemDefault());
}

TEST_CASE("AudioDeviceManager: migration callbacks can be set pre-init",
          "[audio][device][config]") {
    AudioDeviceManager dev;
    // Just verifying that setting callbacks does not crash and stores them.
    REQUIRE_NOTHROW(dev.setBeforeMigration([] {}));
    REQUIRE_NOTHROW(dev.setAfterMigration([] {}));
    REQUIRE_NOTHROW(dev.setBeforeMigration(nullptr));
    REQUIRE_NOTHROW(dev.setAfterMigration(nullptr));
}