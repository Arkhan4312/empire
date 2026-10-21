#include <empire/engine/audio/AudioBusRegistry.h>
#include <empire/engine/audio/AudioResourceCache.h>
#include <empire/engine/audio/AudioVoicePool.h>

#include <algorithm>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "audio_test_helpers.h"

using namespace empire::audio;
using namespace empire::audio::test;
using Catch::Approx;

namespace {

struct NullFixture {
    NullEngine engine;
    AudioBusRegistry buses;

    NullFixture() {
        if (!engine.ready()) SKIP("null backend unavailable");
        REQUIRE(buses.initDefaults().isOk());
        REQUIRE(buses.init(engine.engine()).isOk());
    }
    ~NullFixture() {
        buses.uninit();
    }
};

}  // namespace

// =========================================================================
// Bus registry with real ma_engine
// =========================================================================

TEST_CASE_METHOD(NullFixture, "NullEngine: AudioBusRegistry::init succeeds",
                 "[audio][nullengine][integration]") {
    REQUIRE(buses.size() == 6u);
    REQUIRE(buses.group(buses.find("music")) != nullptr);
}

TEST_CASE_METHOD(NullFixture,
                 "NullEngine: bus volume is propagated to miniaudio",
                 "[audio][nullengine][integration]") {
    auto* g = buses.group(buses.find("music"));
    REQUIRE(g != nullptr);
    const float v = ma_sound_group_get_volume(g->get());
    REQUIRE(v == Approx(0.6f).margin(1e-4f));
}

TEST_CASE_METHOD(NullFixture, "NullEngine: setVolume propagates to miniaudio",
                 "[audio][nullengine][integration]") {
    buses.setVolume(buses.find("music"), 0.25f);
    const float v =
        ma_sound_group_get_volume(buses.group(buses.find("music"))->get());
    REQUIRE(v == Approx(0.25f).margin(1e-4f));
}

TEST_CASE_METHOD(NullFixture, "NullEngine: mute propagates to miniaudio",
                 "[audio][nullengine][integration]") {
    auto* musicGroup = buses.group(buses.find("music"))->get();

    buses.setMuted(buses.find("music"), true);
    REQUIRE(ma_sound_group_get_volume(musicGroup) ==
            Approx(0.0f).margin(1e-4f));

    buses.setMuted(buses.find("music"), false);
    REQUIRE(ma_sound_group_get_volume(musicGroup) ==
            Approx(0.6f).margin(1e-4f));
}

TEST_CASE_METHOD(NullFixture,
                 "NullEngine: solo kills other leaves on the actual engine",
                 "[audio][nullengine][integration][solo]") {
    auto* musicGroup = buses.group(buses.find("music"))->get();
    auto* sfxGroup = buses.group(buses.find("sfx"))->get();

    buses.setSolo(buses.find("music"), true);

    REQUIRE(ma_sound_group_get_volume(musicGroup) ==
            Approx(0.6f).margin(1e-4f));
    REQUIRE(ma_sound_group_get_volume(sfxGroup) == Approx(0.0f).margin(1e-4f));

    buses.clearAllSolo();
    REQUIRE(ma_sound_group_get_volume(sfxGroup) == Approx(1.0f).margin(1e-4f));
}

TEST_CASE_METHOD(NullFixture,
                 "NullEngine: update() drives fade on the actual engine",
                 "[audio][nullengine][integration][fade]") {
    auto* sfxGroup = buses.group(buses.find("sfx"))->get();

    buses.setVolume(buses.find("sfx"), 1.0f);
    buses.applySnapshot({{{"sfx", 0.0f}}}, 1.0f);

    buses.update(0.5f);
    REQUIRE(ma_sound_group_get_volume(sfxGroup) == Approx(0.5f).margin(1e-3f));

    buses.update(0.5f);
    REQUIRE(ma_sound_group_get_volume(sfxGroup) == Approx(0.0f).margin(1e-3f));
}

// =========================================================================
// Voice pool with real engine + resources + groups
// =========================================================================

namespace {

struct VoiceFixture {
    NullEngine engine;
    AudioBusRegistry buses;
    AudioResourceCache cache;
    AudioVoicePool pool;
    ScopedTempDir tmp;
    ScopedAssetRoot root;

    VoiceFixture() : root(tmp.path()) {
        if (!engine.ready()) SKIP("null backend unavailable");
        REQUIRE(buses.initDefaults().isOk());
        REQUIRE(buses.init(engine.engine()).isOk());
        REQUIRE(pool.init(engine.engine()).isOk());
    }
    ~VoiceFixture() {
        pool.uninit();
        buses.uninit();
    }
};

}  // namespace

TEST_CASE_METHOD(VoiceFixture, "NullEngine: start+stop a single voice",
                 "[audio][nullengine][voice][integration]") {
    auto path = writeWav(tmp.path(), "sfx.wav", 1, 22050, 0.1);
    auto desc =
        AudioResourceDesc{.path = path.string(), .mode = LoadMode::Decode};
    auto res = cache.acquire(desc).value();

    VoiceStartDesc vd{};
    vd.resource = res;
    vd.group = buses.group(buses.find("sfx"));
    vd.volume = 1.0f;
    vd.maxConcurrent = 4;
    vd.soundGroupKey = 1;

    auto out = pool.start(vd);
    REQUIRE(out.ok());
    REQUIRE(out.handle.isValid());
    REQUIRE(pool.activeCount() == 1u);
    REQUIRE(pool.isPlaying(out.handle));

    pool.stop(out.handle);
    REQUIRE(pool.activeCount() == 0u);
    REQUIRE_FALSE(pool.isPlaying(out.handle));
}

TEST_CASE_METHOD(VoiceFixture, "NullEngine: per-sound limit reuses oldest",
                 "[audio][nullengine][voice][integration]") {
    auto path = writeWav(tmp.path(), "limited.wav", 1, 22050, 0.2);
    auto desc =
        AudioResourceDesc{.path = path.string(), .mode = LoadMode::Decode};
    auto res = cache.acquire(desc).value();

    VoiceStartDesc vd{};
    vd.resource = res;
    vd.group = buses.group(buses.find("sfx"));
    vd.volume = 1.0f;
    vd.maxConcurrent = 2;
    vd.soundGroupKey = 42;

    auto a = pool.start(vd);
    auto b = pool.start(vd);
    REQUIRE(a.ok());
    REQUIRE(b.ok());
    REQUIRE(pool.activeCount() == 2u);

    auto c = pool.start(vd);
    REQUIRE(c.ok());
    REQUIRE(pool.activeCount() == 2u);
    REQUIRE_FALSE(pool.isPlaying(a.handle));
    REQUIRE(pool.isPlaying(c.handle));
}

TEST_CASE_METHOD(VoiceFixture,
                 "NullEngine: stopByGroupKey releases matching voices",
                 "[audio][nullengine][voice][integration]") {
    auto path1 = writeWav(tmp.path(), "g1.wav", 1, 22050, 0.2);
    auto path2 = writeWav(tmp.path(), "g2.wav", 1, 22050, 0.2);
    auto res1 = cache.acquire({.path = path1.string()}).value();
    auto res2 = cache.acquire({.path = path2.string()}).value();

    VoiceStartDesc vd1{};
    vd1.resource = res1;
    vd1.group = buses.group(buses.find("sfx"));
    vd1.maxConcurrent = 4;
    vd1.soundGroupKey = 100;

    VoiceStartDesc vd2 = vd1;
    vd2.resource = res2;
    vd2.soundGroupKey = 200;

    auto a = pool.start(vd1);
    auto b = pool.start(vd2);
    auto c = pool.start(vd1);
    REQUIRE(pool.activeCount() == 3u);

    pool.stopByGroupKey(100);
    REQUIRE(pool.activeCount() == 1u);
    REQUIRE_FALSE(pool.isPlaying(a.handle));
    REQUIRE_FALSE(pool.isPlaying(c.handle));
    REQUIRE(pool.isPlaying(b.handle));
}