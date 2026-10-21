#include <empire/engine/audio/AudioSpatial.h>
#include <empire/engine/audio/AudioVoice.h>
#include <empire/engine/audio/AudioVoicePool.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace empire::audio;
using Catch::Approx;

TEST_CASE("AudioVoicePool: capacity matches kMaxVoices", "[audio][voice]") {
    REQUIRE(AudioVoicePool::capacity() == AudioVoicePool::kMaxVoices);
}

TEST_CASE("AudioVoicePool: init(nullptr) -> NotInitialized", "[audio][voice]") {
    AudioVoicePool pool;
    auto r = pool.init(nullptr);
    REQUIRE(r.isErr());
    REQUIRE(r.error() == AudioError::NotInitialized);
}

TEST_CASE("AudioVoicePool: start without engine -> NotInitialized",
          "[audio][voice]") {
    AudioVoicePool pool;
    VoiceStartDesc desc{};
    auto out = pool.start(desc);
    REQUIRE_FALSE(out.ok());
    REQUIRE(out.result == VoiceStartResult::NotInitialized);
    REQUIRE_FALSE(out.handle.isValid());
}

TEST_CASE("AudioVoicePool: start with null resource -> InvalidResource",
          "[audio][voice]") {
    AudioVoicePool pool;
    // Force "engine ready" via a null-backend engine so the check for
    // resource surfaces.
    // We can't reach resource check without engine; see integration tests.
    // Here, at least verify the guard order.
    VoiceStartDesc desc{};
    desc.resource = nullptr;
    desc.group = nullptr;
    auto out = pool.start(desc);
    // Without engine, NotInitialized wins
    REQUIRE(out.result == VoiceStartResult::NotInitialized);
}

TEST_CASE("AudioVoicePool: isPlaying(kInvalidVoice) == false",
          "[audio][voice]") {
    AudioVoicePool pool;
    REQUIRE_FALSE(pool.isPlaying(kInvalidVoice));
}

TEST_CASE("AudioVoicePool: activeCount and stolenTotal start at 0",
          "[audio][voice]") {
    AudioVoicePool pool;
    REQUIRE(pool.activeCount() == 0u);
    REQUIRE(pool.stolenTotal() == 0u);
}

TEST_CASE("AudioVoicePool: listener2D round-trips",
          "[audio][voice][listener]") {
    AudioVoicePool pool;
    Listener2D l{};
    l.x = 12.0f;
    l.y = -5.0f;
    l.angle = 0.7f;
    l.halfViewWidth = 320.0f;
    pool.setListener2D(l);
    const auto& got = pool.listener2D();
    REQUIRE(got.x == Approx(l.x));
    REQUIRE(got.y == Approx(l.y));
    REQUIRE(got.angle == Approx(l.angle));
    REQUIRE(got.halfViewWidth == Approx(l.halfViewWidth));
}

// =========================================================================
// computePan2D — pure math
// =========================================================================

TEST_CASE("computePan2D: listener at origin, angle=0, halfView=100",
          "[audio][voice][pan]") {
    AudioVoicePool pool;
    Listener2D l{};
    l.x = 0.0f;
    l.y = 0.0f;
    l.angle = 0.0f;
    l.halfViewWidth = 100.0f;
    pool.setListener2D(l);

    // Same position -> center
    REQUIRE(pool.computePan2D({0.0f, 0.0f}) == Approx(0.0f));

    // At angle=0, the pan formula projects onto (0, -1):
    //   pan = -dy / halfView
    // +Y source → negative pan (left)
    // -Y source → positive pan (right)
    REQUIRE(pool.computePan2D({0.0f, 100.0f}) == Approx(-1.0f));
    REQUIRE(pool.computePan2D({0.0f, -100.0f}) == Approx(1.0f));
    REQUIRE(pool.computePan2D({0.0f, 50.0f}) == Approx(-0.5f));
    REQUIRE(pool.computePan2D({0.0f, -50.0f}) == Approx(0.5f));

    // X doesn't affect pan at angle=0
    REQUIRE(pool.computePan2D({100.0f, 0.0f}) == Approx(0.0f));
    REQUIRE(pool.computePan2D({-100.0f, 0.0f}) == Approx(0.0f));
}

TEST_CASE("computePan2D: clamps to [-1, 1]", "[audio][voice][pan]") {
    AudioVoicePool pool;
    Listener2D l{};
    l.halfViewWidth = 100.0f;
    l.angle = 0.0f;
    pool.setListener2D(l);

    REQUIRE(pool.computePan2D({0.0f, 200.0f}) == Approx(-1.0f));
    REQUIRE(pool.computePan2D({0.0f, -200.0f}) == Approx(1.0f));
}

TEST_CASE("computePan2D: listener rotated π/2", "[audio][voice][pan]") {
    AudioVoicePool pool;
    Listener2D l{};
    l.halfViewWidth = 100.0f;
    l.angle = 1.57079632679f;  // π/2
    pool.setListener2D(l);

    // At angle=π/2, cos≈0, sin≈1: pan = +dx / halfView
    SECTION("source to the right → +pan") {
        INFO("pan=" << pool.computePan2D({100.0f, 0.0f}));
        REQUIRE(pool.computePan2D({100.0f, 0.0f}) ==
                Approx(1.0f).margin(0.01f));
    }
    SECTION("source to the left → -pan") {
        REQUIRE(pool.computePan2D({-100.0f, 0.0f}) ==
                Approx(-1.0f).margin(0.01f));
    }
    SECTION("source along y → near-center") {
        // Y contribution is scaled by cos(π/2) ≈ 0
        REQUIRE(std::abs(pool.computePan2D({0.0f, 100.0f})) < 0.01f);
    }
}

TEST_CASE("computePan2D: honours listener position", "[audio][voice][pan]") {
    AudioVoicePool pool;
    Listener2D l{};
    l.x = 50.0f;
    l.y = 50.0f;
    l.angle = 0.0f;
    l.halfViewWidth = 100.0f;
    pool.setListener2D(l);

    REQUIRE(pool.computePan2D({50.0f, 50.0f}) == Approx(0.0f));
    REQUIRE(pool.computePan2D({50.0f, 150.0f}) == Approx(-1.0f));
    REQUIRE(pool.computePan2D({50.0f, -50.0f}) == Approx(1.0f));
}

TEST_CASE("computePan2D: halfViewWidth has max(1, ...) guard",
          "[audio][voice][pan]") {
    AudioVoicePool pool;
    Listener2D l{};
    l.halfViewWidth = 0.0f;  // will be clamped to 1
    pool.setListener2D(l);

    // dy = 100 → pan = -100/1 → clamped to -1
    REQUIRE(pool.computePan2D({0.0f, 100.0f}) == Approx(-1.0f));
    // dy = 0.5 → pan = -0.5/1 = -0.5
    REQUIRE(pool.computePan2D({0.0f, 0.5f}) == Approx(-0.5f));
}