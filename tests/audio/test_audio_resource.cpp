#include <empire/engine/audio/AudioResource.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

#include "audio_test_helpers.h"

using namespace empire::audio;
using namespace empire::audio::test;
using Catch::Approx;

TEST_CASE("AudioResource: default-constructed is Unloaded",
          "[audio][resource]") {
    AudioResource res;
    REQUIRE(res.state() == ResourceState::Unloaded);
    REQUIRE_FALSE(res.isReady());
    REQUIRE_FALSE(res.isLoading());
    REQUIRE_FALSE(res.isStreamed());
    REQUIRE_FALSE(res.isDecoded());
    REQUIRE(res.pcmData() == nullptr);
    REQUIRE(res.streamPath() == nullptr);
    REQUIRE(res.frameCount() == 0u);
}

TEST_CASE("AudioResource: empty path -> InvalidArgument", "[audio][resource]") {
    AudioResource res;
    auto r = res.load({.path = ""});
    REQUIRE(r.isErr());
    REQUIRE(r.error() == AudioError::InvalidArgument);
    REQUIRE(res.state() == ResourceState::Failed);
}

TEST_CASE("AudioResource: missing file -> FileNotFound", "[audio][resource]") {
    ScopedTempDir tmp;
    ScopedAssetRoot root(tmp.path());

    AudioResource res;
    auto r = res.load({.path = "nope.wav"});
    REQUIRE(r.isErr());
    REQUIRE(r.error() == AudioError::FileNotFound);
    REQUIRE(res.state() == ResourceState::Failed);
}

TEST_CASE("AudioResource: corrupt file fails load", "[audio][resource]") {
    ScopedTempDir tmp;
    ScopedAssetRoot root(tmp.path());
    writeFile(tmp.path(), "bad.wav", std::string{"this is not a wav file"});

    AudioResource res;
    auto r = res.load({.path = "bad.wav"});
    REQUIRE(r.isErr());
    REQUIRE(res.state() == ResourceState::Failed);
    REQUIRE_FALSE(res.isReady());
}

TEST_CASE("AudioResource: decodes WAV (mono, LoadMode::Decode)",
          "[audio][resource]") {
    ScopedTempDir tmp;
    ScopedAssetRoot root(tmp.path());
    writeWav(tmp.path(), "tone.wav", /*channels=*/1, /*sampleRate=*/44100,
             /*durationSec=*/0.1);

    AudioResource res;
    auto r = res.load({.path = "tone.wav", .mode = LoadMode::Decode});
    REQUIRE(r.isOk());
    REQUIRE(res.state() == ResourceState::Ready);
    REQUIRE(res.isReady());
    REQUIRE(res.isDecoded());
    REQUIRE_FALSE(res.isStreamed());
    REQUIRE(res.channels() == 1u);
    REQUIRE(res.sampleRate() == 44100u);
    REQUIRE(res.format() == ma_format_f32);
    REQUIRE(res.frameCount() > 0u);
    REQUIRE(res.pcmData() != nullptr);
    REQUIRE(res.streamPath() == nullptr);
    REQUIRE_FALSE(res.logicalPath().empty());
    REQUIRE_FALSE(res.resolvedPath().empty());
}

TEST_CASE("AudioResource: decodes WAV (stereo)", "[audio][resource]") {
    ScopedTempDir tmp;
    ScopedAssetRoot root(tmp.path());
    writeWav(tmp.path(), "stereo.wav", /*channels=*/2, /*sampleRate=*/22050,
             /*durationSec=*/0.05);

    AudioResource res;
    REQUIRE(res.load({.path = "stereo.wav", .mode = LoadMode::Decode}).isOk());
    REQUIRE(res.channels() == 2u);
    REQUIRE(res.sampleRate() == 22050u);
    REQUIRE(res.frameCount() > 0u);
}

TEST_CASE("AudioResource: PCM samples are within [-1, 1] after f32 convert",
          "[audio][resource]") {
    ScopedTempDir tmp;
    ScopedAssetRoot root(tmp.path());
    writeWav(tmp.path(), "tone.wav", 1, 44100, 0.05);

    AudioResource res;
    REQUIRE(res.load({.path = "tone.wav", .mode = LoadMode::Decode}).isOk());
    REQUIRE(res.format() == ma_format_f32);

    const auto* samples = static_cast<const float*>(res.pcmData());
    const auto total = res.frameCount() * res.channels();
    REQUIRE(total > 0u);
    for (ma_uint64 i = 0; i < total; ++i) {
        REQUIRE(std::isfinite(samples[i]));
        REQUIRE(std::fabs(samples[i]) <= 1.0f + 1e-5f);
    }
}

TEST_CASE("AudioResource: LoadMode::Stream keeps pcmData empty",
          "[audio][resource][stream]") {
    ScopedTempDir tmp;
    ScopedAssetRoot root(tmp.path());
    writeWav(tmp.path(), "stream.wav", 1, 44100, 0.2);

    AudioResource res;
    auto r = res.load({.path = "stream.wav", .mode = LoadMode::Stream});
    REQUIRE(r.isOk());
    REQUIRE(res.isReady());
    REQUIRE(res.isStreamed());
    REQUIRE_FALSE(res.isDecoded());
    REQUIRE(res.pcmData() == nullptr);
    REQUIRE(res.streamPath() != nullptr);
    REQUIRE(res.frameCount() > 0u);
}

TEST_CASE("AudioResource: LoadMode::Auto picks Decode for .wav",
          "[audio][resource][auto]") {
    ScopedTempDir tmp;
    ScopedAssetRoot root(tmp.path());
    writeWav(tmp.path(), "auto.wav", 1, 22050, 0.05);

    AudioResource res;
    REQUIRE(res.load({.path = "auto.wav", .mode = LoadMode::Auto}).isOk());
    REQUIRE(res.isDecoded());
}

TEST_CASE("AudioResource: empty WAV (0 samples) fails decode",
          "[audio][resource]") {
    ScopedTempDir tmp;
    ScopedAssetRoot root(tmp.path());
    // Header-only WAV, no data
    const auto bytes = makeWavBytes(1, 44100, 0.0);
    writeFile(tmp.path(), "empty.wav", bytes);

    AudioResource res;
    auto r = res.load({.path = "empty.wav"});
    REQUIRE(r.isErr());
    REQUIRE(res.state() == ResourceState::Failed);
}

TEST_CASE("AudioResource: reloading replaces data", "[audio][resource]") {
    ScopedTempDir tmp;
    ScopedAssetRoot root(tmp.path());
    writeWav(tmp.path(), "a.wav", 1, 22050, 0.1);

    AudioResource res;
    REQUIRE(res.load({.path = "a.wav"}).isOk());
    const auto frames1 = res.frameCount();
    REQUIRE(frames1 > 0u);

    // Change duration, reload
    writeWav(tmp.path(), "a.wav", 1, 22050, 0.2);
    REQUIRE(res.load({.path = "a.wav"}).isOk());
    REQUIRE(res.frameCount() > frames1);
}

TEST_CASE("AudioResource: move construction transfers state",
          "[audio][resource][move]") {
    ScopedTempDir tmp;
    ScopedAssetRoot root(tmp.path());
    writeWav(tmp.path(), "mv.wav", 1, 44100, 0.05);

    AudioResource a;
    REQUIRE(a.load({.path = "mv.wav"}).isOk());
    const void* data = a.pcmData();
    const ma_uint64 frames = a.frameCount();

    AudioResource b(std::move(a));
    REQUIRE(b.isReady());
    REQUIRE(b.pcmData() == data);
    REQUIRE(b.frameCount() == frames);

    REQUIRE(a.state() == ResourceState::Unloaded);
    REQUIRE(a.pcmData() == nullptr);
}

TEST_CASE("AudioResource: uninit resets all state", "[audio][resource]") {
    ScopedTempDir tmp;
    ScopedAssetRoot root(tmp.path());
    writeWav(tmp.path(), "u.wav", 1, 22050, 0.05);

    AudioResource res;
    REQUIRE(res.load({.path = "u.wav"}).isOk());
    REQUIRE(res.isReady());

    res.uninit();
    REQUIRE(res.state() == ResourceState::Unloaded);
    REQUIRE_FALSE(res.isReady());
    REQUIRE(res.pcmData() == nullptr);
    REQUIRE(res.frameCount() == 0u);
    REQUIRE(res.channels() == 0u);
    REQUIRE(res.sampleRate() == 0u);
    REQUIRE(res.format() == ma_format_unknown);
}

TEST_CASE("AudioResource: markLoading sets Loading state",
          "[audio][resource]") {
    AudioResource res;
    res.markLoading();
    REQUIRE(res.isLoading());
    REQUIRE_FALSE(res.isReady());
    REQUIRE(res.state() == ResourceState::Loading);
}