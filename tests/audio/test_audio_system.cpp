#include <empire/engine/audio/AudioSystem.h>

#include <catch2/catch_test_macros.hpp>

using namespace empire::audio;

TEST_CASE("AudioSystem: not ready before init", "[audio][system][preinit]") {
    AudioSystem system;
    REQUIRE_FALSE(system.ready());
    REQUIRE_FALSE(system.paused());
}

TEST_CASE("AudioSystem: stats before init are zeroed",
          "[audio][system][preinit]") {
    AudioSystem system;
    const auto s = system.stats();
    REQUIRE_FALSE(s.deviceReady);
    REQUIRE(s.currentDeviceIndex == -1);
    REQUIRE(s.voicesActive == 0u);
    REQUIRE(s.voicesCapacity == AudioVoicePool::capacity());
    REQUIRE(s.busCount == 0u);
    REQUIRE(s.cachedResources == 0u);
    REQUIRE_FALSE(s.musicPlaying);
    REQUIRE_FALSE(s.musicPaused);
    REQUIRE(s.eventTotal == 0u);
}

TEST_CASE("AudioSystem: diagnosticsJson before init is valid shape",
          "[audio][system][preinit]") {
    AudioSystem system;
    const auto json = system.diagnosticsJson();
    REQUIRE_FALSE(json.empty());
    // Documented top-level keys must appear
    REQUIRE(json.find("\"device\"") != std::string::npos);
    REQUIRE(json.find("\"voices\"") != std::string::npos);
    REQUIRE(json.find("\"buses\"") != std::string::npos);
    REQUIRE(json.find("\"resources\"") != std::string::npos);
    REQUIRE(json.find("\"music\"") != std::string::npos);
    REQUIRE(json.find("\"events\"") != std::string::npos);
}

TEST_CASE("AudioSystem: update pre-init is safe", "[audio][system][preinit]") {
    AudioSystem system;
    REQUIRE_NOTHROW(system.update(0.0f));
    REQUIRE_NOTHROW(system.update(0.016f));
    REQUIRE_NOTHROW(system.update(1.0f));
}

TEST_CASE("AudioSystem: setPaused pre-init is safe",
          "[audio][system][preinit]") {
    AudioSystem system;
    REQUIRE_NOTHROW(system.setPaused(true));
    REQUIRE_FALSE(system.paused());
    REQUIRE_NOTHROW(system.setPaused(false));
}

TEST_CASE("AudioSystem: setEventBus before init stores pointer",
          "[audio][system][preinit]") {
    AudioSystem system;
    REQUIRE_NOTHROW(system.setEventBus(nullptr));
}

TEST_CASE("AudioSystem: sound operations pre-init behave gracefully",
          "[audio][system][preinit]") {
    AudioSystem system;

    // Loading a sound without a device: loadSound should either fail or
    // succeed purely through resourceCache (which does not need a device).
    // Either behaviour is acceptable; must not crash.
    SoundDesc desc{};
    desc.resource.path = "never_exists.wav";
    auto r = system.loadSound("x", desc);
    REQUIRE(r.isErr());

    REQUIRE_FALSE(system.isSoundReady(kInvalidSound));
    REQUIRE(system.soundName(kInvalidSound).empty());
}

TEST_CASE("AudioSystem: playback pre-init returns invalid handle",
          "[audio][system][preinit]") {
    AudioSystem system;
    REQUIRE_FALSE(system.play(kInvalidSound).isValid());
    REQUIRE_FALSE(system.playAt2D(kInvalidSound, 0.0f, 0.0f).isValid());
    REQUIRE_FALSE(system.playAt3D(kInvalidSound, Spatial3D{}).isValid());
    REQUIRE_FALSE(system.isVoicePlaying(kInvalidVoice));
    REQUIRE(system.activeVoiceCount() == 0u);
    REQUIRE(system.stolenVoiceCount() == 0u);
}

TEST_CASE("AudioSystem: volume setters pre-init are safe",
          "[audio][system][preinit]") {
    AudioSystem system;
    REQUIRE_NOTHROW(system.setMasterVolume(0.5f));
    REQUIRE_NOTHROW(system.setSfxVolume(0.5f));
    REQUIRE_NOTHROW(system.setMusicVolume(0.5f));
    // Before init, buses map is empty → getters return 0
    REQUIRE(system.masterVolume() == 0.0f);
    REQUIRE(system.sfxVolume() == 0.0f);
    REQUIRE(system.musicVolume() == 0.0f);
}

TEST_CASE("AudioSystem: device helpers pre-init", "[audio][system][preinit]") {
    AudioSystem system;
    REQUIRE(system.listOutputDevices().empty());
    REQUIRE(system.currentOutputDevice() == -1);
    REQUIRE_FALSE(system.setOutputDevice(0));
    REQUIRE_FALSE(system.setOutputDevice(-1));
}

TEST_CASE("AudioSystem: shutdown before init is safe",
          "[audio][system][preinit]") {
    AudioSystem system;
    REQUIRE_NOTHROW(system.shutdown());
    REQUIRE_FALSE(system.ready());
}