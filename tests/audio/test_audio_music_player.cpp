#include <empire/engine/audio/AudioBusRegistry.h>
#include <empire/engine/audio/AudioDeviceManager.h>
#include <empire/engine/audio/AudioMusicPlayer.h>

#include <catch2/catch_test_macros.hpp>

#include "audio_test_helpers.h"

using namespace empire::audio;
using namespace empire::audio::test;

namespace {

struct MusicFixture {
    AudioDeviceManager device;
    AudioBusRegistry buses;
    AudioMusicPlayer music;

    MusicFixture() {
        // Do NOT call device.init() — device.engine() stays null and the
        // player's `ready()` is false. We exercise state-only paths.
        REQUIRE(music.init(device, buses).isOk());
    }
    ~MusicFixture() {
        music.uninit();
    }
};

}  // namespace

TEST_CASE_METHOD(MusicFixture, "AudioMusicPlayer: initially idle",
                 "[audio][music][state]") {
    REQUIRE_FALSE(music.isPlaying());
    REQUIRE_FALSE(music.isPaused());
    REQUIRE(music.currentPath().empty());
    REQUIRE(music.queuedCount() == 0u);
    REQUIRE(music.currentFrame() == 0u);
}

TEST_CASE_METHOD(MusicFixture,
                 "AudioMusicPlayer: play() is a no-op without a live engine",
                 "[audio][music][state]") {
    // No device engine → ready() is false → play must be safe & silent
    REQUIRE_NOTHROW(music.play("anything.wav"));
    REQUIRE_FALSE(music.isPlaying());
    REQUIRE(music.currentPath().empty());
}

TEST_CASE_METHOD(MusicFixture,
                 "AudioMusicPlayer: playNext queues regardless of readiness",
                 "[audio][music][queue]") {
    music.playNext("a.wav");
    music.playNext("b.wav", /*fadeInSec=*/1.5f, /*loop=*/false);
    REQUIRE(music.queuedCount() == 2u);

    music.clearQueue();
    REQUIRE(music.queuedCount() == 0u);
}

TEST_CASE_METHOD(MusicFixture, "AudioMusicPlayer: playNext ignores empty path",
                 "[audio][music][queue]") {
    music.playNext("");
    REQUIRE(music.queuedCount() == 0u);
}

TEST_CASE_METHOD(MusicFixture,
                 "AudioMusicPlayer: playNext accepts varied options",
                 "[audio][music][queue]") {
    music.playNext("t.wav", 0.0f, /*loop=*/false);
    REQUIRE(music.queuedCount() == 1u);
}

TEST_CASE_METHOD(MusicFixture, "AudioMusicPlayer: stop() on idle is safe",
                 "[audio][music][state]") {
    REQUIRE_NOTHROW(music.stop());
    REQUIRE_NOTHROW(music.stop(0.0f));
    REQUIRE_FALSE(music.isPlaying());
}

TEST_CASE_METHOD(MusicFixture,
                 "AudioMusicPlayer: pause/resume on idle toggles flag",
                 "[audio][music][pause]") {
    // music.pause() sets musicPaused = true, then iterates slots.
    music.pause(0.0f);
    REQUIRE(music.isPaused());

    music.resume(0.0f);
    REQUIRE_FALSE(music.isPaused());
}

TEST_CASE_METHOD(MusicFixture,
                 "AudioMusicPlayer: loop-points round-trip (state only)",
                 "[audio][music][loop]") {
    // We can set/clear loop points; they only affect the next play().
    REQUIRE_NOTHROW(music.setLoopPoints(100, 500));
    REQUIRE_NOTHROW(music.clearLoopPoints());
    // No observable getter for loop points; behaviour test needs real engine.
}

TEST_CASE_METHOD(MusicFixture, "AudioMusicPlayer: update() is safe while idle",
                 "[audio][music][state]") {
    REQUIRE_NOTHROW(music.update(0.0f));
    REQUIRE_NOTHROW(music.update(0.016f));
    REQUIRE_NOTHROW(music.update(1.0f));
    REQUIRE_FALSE(music.isPlaying());
}

TEST_CASE_METHOD(MusicFixture,
                 "AudioMusicPlayer: crossfadeTo is a no-op without engine",
                 "[audio][music][state]") {
    REQUIRE_NOTHROW(music.crossfadeTo("x.wav", 2.0f, /*loop=*/true));
    REQUIRE_FALSE(music.isPlaying());
}

TEST_CASE_METHOD(MusicFixture,
                 "AudioMusicPlayer: migration hooks are safe without engine",
                 "[audio][music][migration]") {
    REQUIRE_NOTHROW(music.onBeforeMigration());
    REQUIRE_NOTHROW(music.onAfterMigration());
    REQUIRE_FALSE(music.isPlaying());
}

TEST_CASE_METHOD(MusicFixture, "AudioMusicPlayer: uninit clears state",
                 "[audio][music][uninit]") {
    music.playNext("a.wav");
    music.playNext("b.wav");
    REQUIRE(music.queuedCount() == 2u);

    music.uninit();
    REQUIRE(music.queuedCount() == 0u);
    REQUIRE_FALSE(music.isPaused());
    REQUIRE(music.currentPath().empty());
    REQUIRE_FALSE(music.isPlaying());
}