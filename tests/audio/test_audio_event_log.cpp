#include <empire/engine/audio/AudioEventLog.h>

#include <catch2/catch_test_macros.hpp>
#include <thread>
#include <vector>

using namespace empire::audio;

TEST_CASE("AudioEventLog: initially empty", "[audio][eventlog]") {
    AudioEventLog log;
    REQUIRE(log.size() == 0u);
    REQUIRE(log.totalCount() == 0u);
    REQUIRE(log.snapshot().empty());
}

TEST_CASE("AudioEventLog: push increments size and total",
          "[audio][eventlog]") {
    AudioEventLog log;
    log.push(AudioEventKind::DeviceLost, AudioError::Unknown, 1, 2);
    REQUIRE(log.size() == 1u);
    REQUIRE(log.totalCount() == 1u);

    log.push(AudioEventKind::VoiceStolen, AudioError::Unknown, 3, 4);
    REQUIRE(log.size() == 2u);
    REQUIRE(log.totalCount() == 2u);
}

TEST_CASE("AudioEventLog: snapshot preserves order and content",
          "[audio][eventlog]") {
    AudioEventLog log;
    for (std::uint32_t i = 0; i < 5; ++i) {
        log.push(AudioEventKind::VoiceStolen, AudioError::Unknown, i, i * 2);
    }
    const auto snap = log.snapshot();
    REQUIRE(snap.size() == 5u);
    for (std::uint32_t i = 0; i < 5; ++i) {
        REQUIRE(snap[i].a == i);
        REQUIRE(snap[i].b == i * 2);
        REQUIRE(snap[i].kind == AudioEventKind::VoiceStolen);
    }
    // Monotonic ticks
    for (std::size_t i = 1; i < snap.size(); ++i) {
        REQUIRE(snap[i].tick == snap[i - 1].tick + 1);
    }
}

TEST_CASE("AudioEventLog: ring buffer keeps most recent entries",
          "[audio][eventlog]") {
    AudioEventLog log;
    constexpr std::uint32_t kExtra = 10;
    const auto total = AudioEventLog::kCapacity + kExtra;
    for (std::uint32_t i = 0; i < total; ++i) {
        log.push(AudioEventKind::VoiceStolen, AudioError::Unknown, i, 0);
    }

    REQUIRE(log.size() == AudioEventLog::kCapacity);
    REQUIRE(log.totalCount() == total);

    const auto snap = log.snapshot();
    REQUIRE(snap.size() == AudioEventLog::kCapacity);
    // Oldest surviving entry is index kExtra
    REQUIRE(snap.front().a == kExtra);
    // Newest is total - 1
    REQUIRE(snap.back().a == total - 1);
}

TEST_CASE("AudioEventLog: totalCount grows unbounded past capacity",
          "[audio][eventlog]") {
    AudioEventLog log;
    for (std::size_t i = 0; i < AudioEventLog::kCapacity * 2 + 5; ++i) {
        log.push(AudioEventKind::VoiceStolen);
    }
    REQUIRE(log.totalCount() == AudioEventLog::kCapacity * 2 + 5);
    REQUIRE(log.size() == AudioEventLog::kCapacity);
}

TEST_CASE("AudioEventLog: hashStr matches FNV-1a 32-bit",
          "[audio][eventlog][hash]") {
    // Known FNV-1a test vectors
    REQUIRE(AudioEventLog::hashStr("") == 2166136261u);  // 0x811C9DC5
    REQUIRE(AudioEventLog::hashStr("a") == 0xE40C292Cu);
    REQUIRE(AudioEventLog::hashStr("foobar") == 0xBF9CF968u);
}

TEST_CASE("AudioEventLog: hashStr deterministic and distinct",
          "[audio][eventlog][hash]") {
    REQUIRE(AudioEventLog::hashStr("sfx/click.wav") ==
            AudioEventLog::hashStr("sfx/click.wav"));
    REQUIRE(AudioEventLog::hashStr("sfx/click.wav") !=
            AudioEventLog::hashStr("sfx/click2.wav"));
    // Different lengths also differ
    REQUIRE(AudioEventLog::hashStr("a") != AudioEventLog::hashStr("aa"));
}

TEST_CASE("AudioEventLog: concurrent push keeps total consistent",
          "[audio][eventlog][stress]") {
    constexpr int kThreads = 4;
    constexpr int kPerThread = 500;

    AudioEventLog log;
    std::vector<std::thread> workers;
    workers.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i) {
        workers.emplace_back([&] {
            for (int j = 0; j < kPerThread; ++j) {
                log.push(AudioEventKind::VoiceStolen);
            }
        });
    }
    for (auto& t : workers) t.join();

    REQUIRE(log.totalCount() ==
            static_cast<std::uint64_t>(kThreads) * kPerThread);
    REQUIRE(log.size() == AudioEventLog::kCapacity);
    // Snapshot is a valid window of ticks
    const auto snap = log.snapshot();
    REQUIRE(snap.size() == AudioEventLog::kCapacity);
    for (std::size_t i = 1; i < snap.size(); ++i) {
        REQUIRE(snap[i].tick == snap[i - 1].tick + 1);
    }
}