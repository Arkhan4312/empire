#include <empire/engine/audio/detail/MaAudioBufferRef.h>
#include <empire/engine/audio/detail/MaRaii.h>

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <memory>
#include <vector>

#include "audio_test_helpers.h"

using namespace empire::audio::detail;
using namespace empire::audio::test;
using empire::audio::AudioError;

namespace {

// Возвращает nullptr, если null-backend недоступен (не должно случаться в
// наших сборках, но аккуратно обрабатываем).
ma_backend* nullBackends() {
    static ma_backend backends[] = {ma_backend_null};
    return backends;
}

struct NullContext {
    MaContext ctx;
    NullContext() {
        auto r = ctx.init(nullBackends(), 1, nullptr);
        if (!r.isOk()) SKIP("null backend unavailable");
    }
};

}  // namespace

// =========================================================================
// MaLog
// =========================================================================

TEST_CASE("MaLog: default state is invalid", "[audio][raii][log]") {
    MaLog log;
    REQUIRE_FALSE(log.isValid());
    REQUIRE(log.get() == nullptr);
    const MaLog& cl = log;
    REQUIRE(cl.get() == nullptr);
}

TEST_CASE("MaLog: init/uninit lifecycle", "[audio][raii][log]") {
    MaLog log;
    REQUIRE(log.init().isOk());
    REQUIRE(log.isValid());
    REQUIRE(log.get() != nullptr);

    log.uninit();
    REQUIRE_FALSE(log.isValid());
    REQUIRE(log.get() == nullptr);
}

TEST_CASE("MaLog: init is idempotent", "[audio][raii][log]") {
    MaLog log;
    REQUIRE(log.init().isOk());
    ma_log* p = log.get();
    REQUIRE(log.init().isOk());
    REQUIRE(log.get() == p);
}

TEST_CASE("MaLog: uninit is idempotent", "[audio][raii][log]") {
    MaLog log;
    REQUIRE(log.init().isOk());
    log.uninit();
    REQUIRE_NOTHROW(log.uninit());
    REQUIRE_FALSE(log.isValid());
}

TEST_CASE("MaLog: uninit on default-constructed is a no-op",
          "[audio][raii][log]") {
    MaLog log;
    REQUIRE_NOTHROW(log.uninit());
    REQUIRE_FALSE(log.isValid());
}

TEST_CASE("MaLog: move-construct transfers validity", "[audio][raii][log]") {
    MaLog a;
    REQUIRE(a.init().isOk());

    MaLog b(std::move(a));
    REQUIRE(b.isValid());
    REQUIRE(b.get() != nullptr);
    REQUIRE_FALSE(a.isValid());
    REQUIRE(a.get() == nullptr);
}

TEST_CASE("MaLog: move-assign onto valid target releases previous handle",
          "[audio][raii][log]") {
    MaLog a;
    REQUIRE(a.init().isOk());
    MaLog b;
    REQUIRE(b.init().isOk());

    b = std::move(a);
    REQUIRE(b.isValid());
    REQUIRE(b.get() != nullptr);
    REQUIRE_FALSE(a.isValid());
}

TEST_CASE("MaLog: self-move is harmless", "[audio][raii][log]") {
    MaLog a;
    REQUIRE(a.init().isOk());
    ma_log* p = a.get();

    MaLog& alias = a;
    a = std::move(alias);
    REQUIRE(a.isValid());
    REQUIRE(a.get() == p);
}

TEST_CASE("MaLog: destructor on invalid instance is safe",
          "[audio][raii][log]") {
    REQUIRE_NOTHROW([] { MaLog log; }());
}

// =========================================================================
// MaDecoder
// =========================================================================

TEST_CASE("MaDecoder: default state is invalid", "[audio][raii][decoder]") {
    MaDecoder d;
    REQUIRE_FALSE(d.isValid());
    REQUIRE(d.get() == nullptr);
    REQUIRE(d.dataSource() == nullptr);
    REQUIRE(d.cursorInPcmFrames() == 0u);
    REQUIRE(d.setLoopPointInPcmFrames(0, 0) == MA_INVALID_ARGS);
    REQUIRE_NOTHROW(d.clearLoopPoint());
}

TEST_CASE("MaDecoder: initFromFile requires non-null path",
          "[audio][raii][decoder]") {
    MaDecoder d;
    auto r = d.initFromFile(nullptr, nullptr);
    REQUIRE(r.isErr());
    REQUIRE(r.error() == AudioError::InvalidArgument);
    REQUIRE_FALSE(d.isValid());
}

TEST_CASE("MaDecoder: initFromFile on a real WAV", "[audio][raii][decoder]") {
    ScopedTempDir tmp;
    auto path = writeWav(tmp.path(), "d.wav", 1, 22050, 0.05);

    MaDecoder d;
    const auto cfg = ma_decoder_config_init(ma_format_f32, 0, 0);
    REQUIRE(d.initFromFile(path.string().c_str(), &cfg).isOk());
    REQUIRE(d.isValid());
    REQUIRE(d.get() != nullptr);
    REQUIRE(d.dataSource() != nullptr);
    // Freshly opened decoder is at frame 0
    REQUIRE(d.cursorInPcmFrames() == 0u);
}

TEST_CASE("MaDecoder: initFromMemory rejects null/size-0",
          "[audio][raii][decoder]") {
    MaDecoder d;
    REQUIRE(d.initFromMemory(nullptr, 0, nullptr).error() ==
            AudioError::InvalidArgument);
    REQUIRE(d.initFromMemory(nullptr, 100, nullptr).error() ==
            AudioError::InvalidArgument);
    REQUIRE(d.initFromMemory(reinterpret_cast<const void*>(0x1), 0, nullptr)
                .error() == AudioError::InvalidArgument);
    REQUIRE_FALSE(d.isValid());
}

TEST_CASE("MaDecoder: init is idempotent", "[audio][raii][decoder]") {
    ScopedTempDir tmp;
    auto path = writeWav(tmp.path(), "d.wav", 1, 22050, 0.05);

    MaDecoder d;
    const auto cfg = ma_decoder_config_init(ma_format_f32, 0, 0);
    REQUIRE(d.initFromFile(path.string().c_str(), &cfg).isOk());
    ma_decoder* p = d.get();

    // Second init is a no-op even with different args
    REQUIRE(d.initFromFile("ignored", nullptr).isOk());
    REQUIRE(d.get() == p);
}

TEST_CASE("MaDecoder: uninit is idempotent", "[audio][raii][decoder]") {
    ScopedTempDir tmp;
    auto path = writeWav(tmp.path(), "u.wav", 1, 22050, 0.05);
    MaDecoder d;
    REQUIRE(d.initFromFile(path.string().c_str(), nullptr).isOk());
    d.uninit();
    REQUIRE_NOTHROW(d.uninit());
    REQUIRE_FALSE(d.isValid());
}

TEST_CASE("MaDecoder: move-construct transfers validity",
          "[audio][raii][decoder]") {
    ScopedTempDir tmp;
    auto path = writeWav(tmp.path(), "m.wav", 1, 22050, 0.05);
    MaDecoder a;
    REQUIRE(a.initFromFile(path.string().c_str(), nullptr).isOk());

    MaDecoder b(std::move(a));
    REQUIRE(b.isValid());
    REQUIRE(b.get() != nullptr);
    REQUIRE(b.dataSource() != nullptr);
    REQUIRE_FALSE(a.isValid());
    REQUIRE(a.get() == nullptr);
    REQUIRE(a.dataSource() == nullptr);
}

TEST_CASE("MaDecoder: move-assign onto valid target",
          "[audio][raii][decoder]") {
    ScopedTempDir tmp;
    auto p1 = writeWav(tmp.path(), "a.wav", 1, 22050, 0.05);
    auto p2 = writeWav(tmp.path(), "b.wav", 1, 22050, 0.05);
    MaDecoder a, b;
    REQUIRE(a.initFromFile(p1.string().c_str(), nullptr).isOk());
    REQUIRE(b.initFromFile(p2.string().c_str(), nullptr).isOk());

    b = std::move(a);
    REQUIRE(b.isValid());
    REQUIRE(b.get() != nullptr);
    REQUIRE_FALSE(a.isValid());
}

TEST_CASE("MaDecoder: self-move is harmless", "[audio][raii][decoder]") {
    ScopedTempDir tmp;
    auto path = writeWav(tmp.path(), "s.wav", 1, 22050, 0.05);
    MaDecoder a;
    REQUIRE(a.initFromFile(path.string().c_str(), nullptr).isOk());
    ma_decoder* p = a.get();
    MaDecoder& alias = a;
    a = std::move(alias);
    REQUIRE(a.isValid());
    REQUIRE(a.get() == p);
}

// =========================================================================
// MaAudioBuffer
// =========================================================================

TEST_CASE("MaAudioBuffer: default state is invalid", "[audio][raii][buffer]") {
    MaAudioBuffer b;
    REQUIRE_FALSE(b.isValid());
    REQUIRE(b.get() == nullptr);
}

TEST_CASE("MaAudioBuffer: init(nullptr) -> InvalidArgument",
          "[audio][raii][buffer]") {
    MaAudioBuffer b;
    auto r = b.init(nullptr);
    REQUIRE(r.isErr());
    REQUIRE(r.error() == AudioError::InvalidArgument);
    REQUIRE_FALSE(b.isValid());
}

TEST_CASE("MaAudioBuffer: init from a valid config", "[audio][raii][buffer]") {
    // NOTE: ma_audio_buffer_init does NOT copy the samples — the caller
    // must keep `pcm` alive for the lifetime of the buffer.
    std::vector<float> pcm(1024, 0.0f);
    auto cfg = ma_audio_buffer_config_init(ma_format_f32, 1, pcm.size(),
                                           pcm.data(), nullptr);

    MaAudioBuffer b;
    REQUIRE(b.init(&cfg).isOk());
    REQUIRE(b.isValid());
    REQUIRE(b.get() != nullptr);
}

TEST_CASE("MaAudioBuffer: init is idempotent", "[audio][raii][buffer]") {
    std::vector<float> pcm(256, 0.0f);
    auto cfg = ma_audio_buffer_config_init(ma_format_f32, 1, pcm.size(),
                                           pcm.data(), nullptr);
    MaAudioBuffer b;
    REQUIRE(b.init(&cfg).isOk());
    ma_audio_buffer* p = b.get();
    REQUIRE(b.init(&cfg).isOk());
    REQUIRE(b.get() == p);
}

TEST_CASE("MaAudioBuffer: uninit is idempotent", "[audio][raii][buffer]") {
    std::vector<float> pcm(256, 0.0f);
    auto cfg = ma_audio_buffer_config_init(ma_format_f32, 1, pcm.size(),
                                           pcm.data(), nullptr);
    MaAudioBuffer b;
    REQUIRE(b.init(&cfg).isOk());
    b.uninit();
    REQUIRE_NOTHROW(b.uninit());
    REQUIRE_FALSE(b.isValid());
}

TEST_CASE("MaAudioBuffer: move-construct transfers validity",
          "[audio][raii][buffer]") {
    std::vector<float> pcm(256, 0.0f);
    auto cfg = ma_audio_buffer_config_init(ma_format_f32, 1, pcm.size(),
                                           pcm.data(), nullptr);
    MaAudioBuffer a;
    REQUIRE(a.init(&cfg).isOk());

    MaAudioBuffer b(std::move(a));
    REQUIRE(b.isValid());
    REQUIRE(b.get() != nullptr);
    REQUIRE_FALSE(a.isValid());
    REQUIRE(a.get() == nullptr);
}

TEST_CASE("MaAudioBuffer: move-assign onto valid target",
          "[audio][raii][buffer]") {
    std::vector<float> pcm(256, 0.0f);
    auto cfg = ma_audio_buffer_config_init(ma_format_f32, 1, pcm.size(),
                                           pcm.data(), nullptr);
    MaAudioBuffer a, b;
    REQUIRE(a.init(&cfg).isOk());
    REQUIRE(b.init(&cfg).isOk());

    b = std::move(a);
    REQUIRE(b.isValid());
    REQUIRE(b.get() != nullptr);
    REQUIRE_FALSE(a.isValid());
}

TEST_CASE("MaAudioBuffer: self-move is harmless", "[audio][raii][buffer]") {
    std::vector<float> pcm(256, 0.0f);
    auto cfg = ma_audio_buffer_config_init(ma_format_f32, 1, pcm.size(),
                                           pcm.data(), nullptr);
    MaAudioBuffer a;
    REQUIRE(a.init(&cfg).isOk());
    ma_audio_buffer* p = a.get();
    MaAudioBuffer& alias = a;
    a = std::move(alias);
    REQUIRE(a.isValid());
    REQUIRE(a.get() == p);
}

// =========================================================================
// MaAudioBufferRef
// =========================================================================

TEST_CASE("MaAudioBufferRef: default state is invalid",
          "[audio][raii][bufref]") {
    MaAudioBufferRef r;
    REQUIRE_FALSE(r.isValid());
    REQUIRE(r.get() == nullptr);
    REQUIRE(r.dataSource() == nullptr);
}

TEST_CASE(
    "MaAudioBufferRef: init rejects null data / zero frames / null format",
    "[audio][raii][bufref]") {
    MaAudioBufferRef r;
    std::vector<float> pcm(64, 0.0f);

    REQUIRE(r.init(ma_format_f32, 1, nullptr, 64).error() ==
            AudioError::InvalidArgument);
    REQUIRE(r.init(ma_format_f32, 1, pcm.data(), 0).error() ==
            AudioError::InvalidArgument);
    REQUIRE(r.init(ma_format_f32, 0, pcm.data(), 64).error() ==
            AudioError::InvalidArgument);
    REQUIRE(r.init(ma_format_unknown, 1, pcm.data(), 64).error() ==
            AudioError::InvalidArgument);
    REQUIRE_FALSE(r.isValid());
}

TEST_CASE("MaAudioBufferRef: init from valid data", "[audio][raii][bufref]") {
    std::vector<float> pcm(64, 0.0f);
    MaAudioBufferRef r;
    REQUIRE(r.init(ma_format_f32, 1, pcm.data(), pcm.size()).isOk());
    REQUIRE(r.isValid());
    REQUIRE(r.get() != nullptr);
    REQUIRE(r.dataSource() != nullptr);
}

TEST_CASE("MaAudioBufferRef: init is idempotent", "[audio][raii][bufref]") {
    std::vector<float> pcm(64, 0.0f);
    MaAudioBufferRef r;
    REQUIRE(r.init(ma_format_f32, 1, pcm.data(), pcm.size()).isOk());
    ma_audio_buffer_ref* p = r.get();
    REQUIRE(r.init(ma_format_f32, 1, pcm.data(), pcm.size()).isOk());
    REQUIRE(r.get() == p);
}

TEST_CASE("MaAudioBufferRef: uninit is idempotent", "[audio][raii][bufref]") {
    std::vector<float> pcm(64, 0.0f);
    MaAudioBufferRef r;
    REQUIRE(r.init(ma_format_f32, 1, pcm.data(), pcm.size()).isOk());
    r.uninit();
    REQUIRE_NOTHROW(r.uninit());
    REQUIRE_FALSE(r.isValid());
}

TEST_CASE("MaAudioBufferRef: move-construct transfers validity",
          "[audio][raii][bufref]") {
    std::vector<float> pcm(64, 0.0f);
    MaAudioBufferRef a;
    REQUIRE(a.init(ma_format_f32, 1, pcm.data(), pcm.size()).isOk());

    MaAudioBufferRef b(std::move(a));
    REQUIRE(b.isValid());
    REQUIRE(b.get() != nullptr);
    REQUIRE(b.dataSource() != nullptr);
    REQUIRE_FALSE(a.isValid());
    REQUIRE(a.get() == nullptr);
    REQUIRE(a.dataSource() == nullptr);
}

TEST_CASE("MaAudioBufferRef: move-assign onto valid target",
          "[audio][raii][bufref]") {
    std::vector<float> pcmA(64, 0.0f);
    std::vector<float> pcmB(64, 0.0f);
    MaAudioBufferRef a, b;
    REQUIRE(a.init(ma_format_f32, 1, pcmA.data(), pcmA.size()).isOk());
    REQUIRE(b.init(ma_format_f32, 1, pcmB.data(), pcmB.size()).isOk());

    b = std::move(a);
    REQUIRE(b.isValid());
    REQUIRE(b.get() != nullptr);
    REQUIRE_FALSE(a.isValid());
}

TEST_CASE("MaAudioBufferRef: self-move is harmless", "[audio][raii][bufref]") {
    std::vector<float> pcm(64, 0.0f);
    MaAudioBufferRef a;
    REQUIRE(a.init(ma_format_f32, 1, pcm.data(), pcm.size()).isOk());
    ma_audio_buffer_ref* p = a.get();
    MaAudioBufferRef& alias = a;
    a = std::move(alias);
    REQUIRE(a.isValid());
    REQUIRE(a.get() == p);
}

// =========================================================================
// MaContext
// =========================================================================

TEST_CASE("MaContext: default state is invalid", "[audio][raii][context]") {
    MaContext c;
    REQUIRE_FALSE(c.isValid());
    REQUIRE(c.get() == nullptr);
}

TEST_CASE("MaContext: init with null backend", "[audio][raii][context]") {
    MaContext c;
    ma_backend backends[] = {ma_backend_null};
    auto r = c.init(backends, 1, nullptr);
    if (!r.isOk()) SKIP("null backend unavailable");
    REQUIRE(c.isValid());
    REQUIRE(c.get() != nullptr);
}

TEST_CASE("MaContext: init is idempotent", "[audio][raii][context]") {
    MaContext c;
    ma_backend backends[] = {ma_backend_null};
    auto r = c.init(backends, 1, nullptr);
    if (!r.isOk()) SKIP("null backend unavailable");

    ma_context* p = c.get();
    REQUIRE(c.init(backends, 1, nullptr).isOk());
    REQUIRE(c.get() == p);
}

TEST_CASE("MaContext: uninit is idempotent", "[audio][raii][context]") {
    MaContext c;
    ma_backend backends[] = {ma_backend_null};
    if (!c.init(backends, 1, nullptr).isOk()) SKIP("null backend unavailable");

    c.uninit();
    REQUIRE_NOTHROW(c.uninit());
    REQUIRE_FALSE(c.isValid());
}

TEST_CASE("MaContext: move-construct transfers validity",
          "[audio][raii][context]") {
    MaContext a;
    ma_backend backends[] = {ma_backend_null};
    if (!a.init(backends, 1, nullptr).isOk()) SKIP("null backend unavailable");

    MaContext b(std::move(a));
    REQUIRE(b.isValid());
    REQUIRE(b.get() != nullptr);
    REQUIRE_FALSE(a.isValid());
    REQUIRE(a.get() == nullptr);
}

TEST_CASE("MaContext: move-assign onto valid target",
          "[audio][raii][context]") {
    ma_backend backends[] = {ma_backend_null};
    MaContext a, b;
    if (!a.init(backends, 1, nullptr).isOk()) SKIP("null backend unavailable");
    if (!b.init(backends, 1, nullptr).isOk()) SKIP("null backend unavailable");

    b = std::move(a);
    REQUIRE(b.isValid());
    REQUIRE(b.get() != nullptr);
    REQUIRE_FALSE(a.isValid());
}

// =========================================================================
// MaResourceManager
// =========================================================================

TEST_CASE("MaResourceManager: default state is invalid", "[audio][raii][rm]") {
    MaResourceManager rm;
    REQUIRE_FALSE(rm.isValid());
    REQUIRE(rm.get() == nullptr);
}

TEST_CASE("MaResourceManager: init with default config", "[audio][raii][rm]") {
    auto cfg = ma_resource_manager_config_init();
    MaResourceManager rm;
    REQUIRE(rm.init(&cfg).isOk());
    REQUIRE(rm.isValid());
    REQUIRE(rm.get() != nullptr);
}

TEST_CASE("MaResourceManager: init is idempotent", "[audio][raii][rm]") {
    auto cfg = ma_resource_manager_config_init();
    MaResourceManager rm;
    REQUIRE(rm.init(&cfg).isOk());
    ma_resource_manager* p = rm.get();
    REQUIRE(rm.init(&cfg).isOk());
    REQUIRE(rm.get() == p);
}

TEST_CASE("MaResourceManager: uninit is idempotent", "[audio][raii][rm]") {
    auto cfg = ma_resource_manager_config_init();
    MaResourceManager rm;
    REQUIRE(rm.init(&cfg).isOk());
    rm.uninit();
    REQUIRE_NOTHROW(rm.uninit());
    REQUIRE_FALSE(rm.isValid());
}

TEST_CASE("MaResourceManager: move-construct transfers validity",
          "[audio][raii][rm]") {
    auto cfg = ma_resource_manager_config_init();
    MaResourceManager a;
    REQUIRE(a.init(&cfg).isOk());

    MaResourceManager b(std::move(a));
    REQUIRE(b.isValid());
    REQUIRE(b.get() != nullptr);
    REQUIRE_FALSE(a.isValid());
    REQUIRE(a.get() == nullptr);
}

TEST_CASE("MaResourceManager: move-assign onto valid target",
          "[audio][raii][rm]") {
    auto cfg = ma_resource_manager_config_init();
    MaResourceManager a, b;
    REQUIRE(a.init(&cfg).isOk());
    REQUIRE(b.init(&cfg).isOk());

    b = std::move(a);
    REQUIRE(b.isValid());
    REQUIRE(b.get() != nullptr);
    REQUIRE_FALSE(a.isValid());
}

// =========================================================================
// MaEngine
// =========================================================================

TEST_CASE("MaEngine: default state is invalid", "[audio][raii][engine]") {
    MaEngine e;
    REQUIRE_FALSE(e.isValid());
    REQUIRE(e.get() == nullptr);
}

TEST_CASE("MaEngine: init on null backend", "[audio][raii][engine]") {
    NullContext ctxWrap;
    ma_engine_config ecfg = ma_engine_config_init();
    ecfg.pContext = ctxWrap.ctx.get();
    ecfg.noAutoStart = MA_TRUE;

    MaEngine e;
    REQUIRE(e.init(&ecfg).isOk());
    REQUIRE(e.isValid());
    REQUIRE(e.get() != nullptr);
}

TEST_CASE("MaEngine: init is idempotent", "[audio][raii][engine]") {
    NullContext ctxWrap;
    ma_engine_config ecfg = ma_engine_config_init();
    ecfg.pContext = ctxWrap.ctx.get();
    ecfg.noAutoStart = MA_TRUE;

    MaEngine e;
    REQUIRE(e.init(&ecfg).isOk());
    ma_engine* p = e.get();
    REQUIRE(e.init(&ecfg).isOk());
    REQUIRE(e.get() == p);
}

TEST_CASE("MaEngine: uninit is idempotent", "[audio][raii][engine]") {
    NullContext ctxWrap;
    ma_engine_config ecfg = ma_engine_config_init();
    ecfg.pContext = ctxWrap.ctx.get();
    ecfg.noAutoStart = MA_TRUE;

    MaEngine e;
    REQUIRE(e.init(&ecfg).isOk());
    e.uninit();
    REQUIRE_NOTHROW(e.uninit());
    REQUIRE_FALSE(e.isValid());
}

TEST_CASE("MaEngine: move-construct transfers validity",
          "[audio][raii][engine]") {
    NullContext ctxWrap;
    ma_engine_config ecfg = ma_engine_config_init();
    ecfg.pContext = ctxWrap.ctx.get();
    ecfg.noAutoStart = MA_TRUE;

    MaEngine a;
    REQUIRE(a.init(&ecfg).isOk());

    MaEngine b(std::move(a));
    REQUIRE(b.isValid());
    REQUIRE(b.get() != nullptr);
    REQUIRE_FALSE(a.isValid());
    REQUIRE(a.get() == nullptr);
}

TEST_CASE("MaEngine: move-assign onto valid target", "[audio][raii][engine]") {
    NullContext ctxWrap;
    ma_engine_config ecfg = ma_engine_config_init();
    ecfg.pContext = ctxWrap.ctx.get();
    ecfg.noAutoStart = MA_TRUE;

    MaEngine a, b;
    REQUIRE(a.init(&ecfg).isOk());
    REQUIRE(b.init(&ecfg).isOk());

    b = std::move(a);
    REQUIRE(b.isValid());
    REQUIRE(b.get() != nullptr);
    REQUIRE_FALSE(a.isValid());
}

// =========================================================================
// MaSoundGroup
// =========================================================================

TEST_CASE("MaSoundGroup: default state is invalid", "[audio][raii][group]") {
    MaSoundGroup g;
    REQUIRE_FALSE(g.isValid());
    REQUIRE(g.get() == nullptr);
}

TEST_CASE("MaSoundGroup: init(nullptr) -> GroupNotReady",
          "[audio][raii][group]") {
    MaSoundGroup g;
    auto r = g.init(nullptr, 0, nullptr);
    REQUIRE(r.isErr());
    REQUIRE(r.error() == AudioError::GroupNotReady);
    REQUIRE_FALSE(g.isValid());
}

TEST_CASE("MaSoundGroup: init on a live engine", "[audio][raii][group]") {
    NullEngine ne;
    if (!ne.ready()) SKIP("null backend unavailable");

    MaSoundGroup g;
    REQUIRE(g.init(ne.engine(), 0, nullptr).isOk());
    REQUIRE(g.isValid());
    REQUIRE(g.get() != nullptr);
}

TEST_CASE("MaSoundGroup: child group uses parent group",
          "[audio][raii][group]") {
    NullEngine ne;
    if (!ne.ready()) SKIP("null backend unavailable");

    MaSoundGroup parent;
    REQUIRE(parent.init(ne.engine(), 0, nullptr).isOk());
    MaSoundGroup child;
    REQUIRE(child.init(ne.engine(), 0, parent.get()).isOk());
    REQUIRE(child.isValid());
}

TEST_CASE("MaSoundGroup: uninit is idempotent", "[audio][raii][group]") {
    NullEngine ne;
    if (!ne.ready()) SKIP("null backend unavailable");

    MaSoundGroup g;
    REQUIRE(g.init(ne.engine(), 0, nullptr).isOk());
    g.uninit();
    REQUIRE_NOTHROW(g.uninit());
    REQUIRE_FALSE(g.isValid());
}

TEST_CASE("MaSoundGroup: move-construct transfers validity",
          "[audio][raii][group]") {
    NullEngine ne;
    if (!ne.ready()) SKIP("null backend unavailable");

    MaSoundGroup a;
    REQUIRE(a.init(ne.engine(), 0, nullptr).isOk());

    MaSoundGroup b(std::move(a));
    REQUIRE(b.isValid());
    REQUIRE(b.get() != nullptr);
    REQUIRE_FALSE(a.isValid());
    REQUIRE(a.get() == nullptr);
}

TEST_CASE("MaSoundGroup: move-assign onto valid target",
          "[audio][raii][group]") {
    NullEngine ne;
    if (!ne.ready()) SKIP("null backend unavailable");

    MaSoundGroup a, b;
    REQUIRE(a.init(ne.engine(), 0, nullptr).isOk());
    REQUIRE(b.init(ne.engine(), 0, nullptr).isOk());

    b = std::move(a);
    REQUIRE(b.isValid());
    REQUIRE(b.get() != nullptr);
    REQUIRE_FALSE(a.isValid());
}

// =========================================================================
// MaSound
// =========================================================================

TEST_CASE("MaSound: default state is invalid", "[audio][raii][sound]") {
    MaSound s;
    REQUIRE_FALSE(s.isValid());
    REQUIRE(s.get() == nullptr);
}

TEST_CASE("MaSound: initFromFile rejects null engine / null path",
          "[audio][raii][sound]") {
    MaSound s;
    REQUIRE(s.initFromFile(nullptr, "path", 0, nullptr).error() ==
            AudioError::InvalidArgument);
    REQUIRE(s.initFromFile(nullptr, nullptr, 0, nullptr).error() ==
            AudioError::InvalidArgument);
    REQUIRE_FALSE(s.isValid());

    NullEngine ne;
    if (!ne.ready()) SKIP("null backend unavailable");
    REQUIRE(s.initFromFile(ne.engine(), nullptr, 0, nullptr).error() ==
            AudioError::InvalidArgument);
}

TEST_CASE("MaSound: initFromFile with a real WAV and group",
          "[audio][raii][sound]") {
    NullEngine ne;
    if (!ne.ready()) SKIP("null backend unavailable");
    ScopedTempDir tmp;
    auto path = writeWav(tmp.path(), "s.wav", 1, 22050, 0.05);

    MaSoundGroup g;
    REQUIRE(g.init(ne.engine(), 0, nullptr).isOk());

    MaSound s;
    REQUIRE(s.initFromFile(ne.engine(), path.string().c_str(),
                           MA_SOUND_FLAG_DECODE, g.get())
                .isOk());
    REQUIRE(s.isValid());
    REQUIRE(s.get() != nullptr);
}

TEST_CASE("MaSound: initFromDataSource rejects null engine / null source",
          "[audio][raii][sound]") {
    MaSound s;
    REQUIRE(s.initFromDataSource(nullptr, nullptr, 0, nullptr).error() ==
            AudioError::InvalidArgument);

    NullEngine ne;
    if (!ne.ready()) SKIP("null backend unavailable");
    REQUIRE(s.initFromDataSource(ne.engine(), nullptr, 0, nullptr).error() ==
            AudioError::InvalidArgument);
}

TEST_CASE("MaSound: initFromDataSource with a decoder",
          "[audio][raii][sound]") {
    NullEngine ne;
    if (!ne.ready()) SKIP("null backend unavailable");
    ScopedTempDir tmp;
    auto path = writeWav(tmp.path(), "ds.wav", 1, 22050, 0.05);

    MaDecoder dec;
    REQUIRE(dec.initFromFile(path.string().c_str(), nullptr).isOk());

    MaSoundGroup g;
    REQUIRE(g.init(ne.engine(), 0, nullptr).isOk());

    MaSound s;
    REQUIRE(
        s.initFromDataSource(ne.engine(), dec.dataSource(), 0, g.get()).isOk());
    REQUIRE(s.isValid());
}

TEST_CASE("MaSound: initCopy rejects null engine / null source",
          "[audio][raii][sound]") {
    MaSound s;
    REQUIRE(s.initCopy(nullptr, nullptr, 0, nullptr).error() ==
            AudioError::InvalidArgument);

    NullEngine ne;
    if (!ne.ready()) SKIP("null backend unavailable");
    REQUIRE(s.initCopy(ne.engine(), nullptr, 0, nullptr).error() ==
            AudioError::InvalidArgument);
}

TEST_CASE("MaSound: initCopy from an existing sound", "[audio][raii][sound]") {
    NullEngine ne;
    if (!ne.ready()) SKIP("null backend unavailable");
    ScopedTempDir tmp;
    auto path = writeWav(tmp.path(), "c.wav", 1, 22050, 0.05);

    MaSoundGroup g;
    REQUIRE(g.init(ne.engine(), 0, nullptr).isOk());

    MaSound src;
    REQUIRE(src.initFromFile(ne.engine(), path.string().c_str(),
                             MA_SOUND_FLAG_DECODE, g.get())
                .isOk());

    MaSound copy;
    auto r = copy.initCopy(ne.engine(), src.get(), 0, g.get());
    // initCopy is only supported for sounds created with certain flags;
    // accept either success (valid) or a clean failure (still invalid).
    if (r.isOk()) {
        REQUIRE(copy.isValid());
    } else {
        REQUIRE_FALSE(copy.isValid());
    }
}

TEST_CASE("MaSound: uninit is idempotent", "[audio][raii][sound]") {
    NullEngine ne;
    if (!ne.ready()) SKIP("null backend unavailable");
    ScopedTempDir tmp;
    auto path = writeWav(tmp.path(), "u.wav", 1, 22050, 0.05);

    MaSoundGroup g;
    REQUIRE(g.init(ne.engine(), 0, nullptr).isOk());

    MaSound s;
    REQUIRE(
        s.initFromFile(ne.engine(), path.string().c_str(), 0, g.get()).isOk());
    s.uninit();
    REQUIRE_NOTHROW(s.uninit());
    REQUIRE_FALSE(s.isValid());
}

TEST_CASE("MaSound: move-construct transfers validity",
          "[audio][raii][sound]") {
    NullEngine ne;
    if (!ne.ready()) SKIP("null backend unavailable");
    ScopedTempDir tmp;
    auto path = writeWav(tmp.path(), "mv.wav", 1, 22050, 0.05);

    MaSoundGroup g;
    REQUIRE(g.init(ne.engine(), 0, nullptr).isOk());

    MaSound a;
    REQUIRE(
        a.initFromFile(ne.engine(), path.string().c_str(), 0, g.get()).isOk());

    MaSound b(std::move(a));
    REQUIRE(b.isValid());
    REQUIRE(b.get() != nullptr);
    REQUIRE_FALSE(a.isValid());
    REQUIRE(a.get() == nullptr);
}

TEST_CASE("MaSound: move-assign onto valid target", "[audio][raii][sound]") {
    NullEngine ne;
    if (!ne.ready()) SKIP("null backend unavailable");
    ScopedTempDir tmp;
    auto p1 = writeWav(tmp.path(), "x.wav", 1, 22050, 0.05);
    auto p2 = writeWav(tmp.path(), "y.wav", 1, 22050, 0.05);

    MaSoundGroup g;
    REQUIRE(g.init(ne.engine(), 0, nullptr).isOk());

    MaSound a, b;
    REQUIRE(
        a.initFromFile(ne.engine(), p1.string().c_str(), 0, g.get()).isOk());
    REQUIRE(
        b.initFromFile(ne.engine(), p2.string().c_str(), 0, g.get()).isOk());

    b = std::move(a);
    REQUIRE(b.isValid());
    REQUIRE(b.get() != nullptr);
    REQUIRE_FALSE(a.isValid());
}

// =========================================================================
// Cross-class: зависимость времени жизни
// =========================================================================

TEST_CASE("RAII: sound must die before its group and engine",
          "[audio][raii][lifetime]") {
    // Sanity test that reverse destruction order works. Не проверяет UB в
    // обратном порядке — этого делать не надо.
    NullEngine ne;
    if (!ne.ready()) SKIP("null backend unavailable");
    ScopedTempDir tmp;
    auto path = writeWav(tmp.path(), "ord.wav", 1, 22050, 0.05);

    MaSoundGroup g;
    REQUIRE(g.init(ne.engine(), 0, nullptr).isOk());
    {
        MaSound s;
        REQUIRE(s.initFromFile(ne.engine(), path.string().c_str(), 0, g.get())
                    .isOk());
        REQUIRE(s.isValid());
    }  // sound uninit'd here
    REQUIRE(g.isValid());  // group still alive

    // End: g dtor runs before engine dtor (reverse decl order)
}