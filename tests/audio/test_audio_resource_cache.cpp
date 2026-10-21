#include <empire/engine/audio/AudioResourceCache.h>

#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <thread>

#include "audio_test_helpers.h"

using namespace empire::audio;
using namespace empire::audio::test;
using namespace std::chrono_literals;

namespace {

AudioResourceDesc makeDesc(const std::filesystem::path& path,
                           LoadMode mode = LoadMode::Decode) {
    return AudioResourceDesc{.path = path.string(), .mode = mode};
}

// Helper: acquire and assert OK, discarding the shared_ptr.
void acquireForSideEffect(AudioResourceCache& cache,
                          const AudioResourceDesc& desc) {
    REQUIRE(cache.acquire(desc).isOk());
}

}  // namespace

TEST_CASE("AudioResourceCache: empty cache has no entries", "[audio][cache]") {
    AudioResourceCache cache;
    REQUIRE(cache.size() == 0u);
    REQUIRE(cache.listLogicalPaths().empty());
    REQUIRE_FALSE(cache.needsReload("anything"));
}

TEST_CASE("AudioResourceCache: acquire loads and caches",
          "[audio][cache][sync]") {
    ScopedTempDir tmp;
    auto path = writeWav(tmp.path(), "one.wav", 1, 22050, 0.05);

    AudioResourceCache cache;
    auto r = cache.acquire(makeDesc(path));
    REQUIRE(r.isOk());
    REQUIRE(r.value()->isReady());
    REQUIRE(cache.size() == 1u);

    auto r2 = cache.acquire(makeDesc(path));
    REQUIRE(r2.isOk());
    REQUIRE(r2.value() == r.value());
    REQUIRE(cache.size() == 1u);
}

TEST_CASE("AudioResourceCache: acquire empty path -> InvalidArgument",
          "[audio][cache]") {
    AudioResourceCache cache;
    auto r = cache.acquire({.path = ""});
    REQUIRE(r.isErr());
    REQUIRE(r.error() == AudioError::InvalidArgument);
    REQUIRE(cache.size() == 0u);
}

TEST_CASE("AudioResourceCache: acquire missing file -> error, no cache entry",
          "[audio][cache]") {
    ScopedTempDir tmp;
    AudioResourceCache cache;
    auto r = cache.acquire(makeDesc(tmp.path() / "missing.wav"));
    REQUIRE(r.isErr());
    REQUIRE(cache.size() == 0u);
}

TEST_CASE("AudioResourceCache: acquireAsync returns immediately Loading",
          "[audio][cache][async]") {
    ScopedTempDir tmp;
    auto path = writeWav(tmp.path(), "a.wav", 1, 44100, 0.5);

    AudioResourceCache cache;
    auto res = cache.acquireAsync(makeDesc(path));
    REQUIRE(res != nullptr);
    REQUIRE((res->isLoading() || res->isReady()));

    auto completions = waitForCompletion(cache, path.string());
    REQUIRE(completions.size() == 1u);
    REQUIRE_FALSE(completions[0].wasReload);
    REQUIRE(completions[0].resource->isReady());
    REQUIRE(res->isReady());
}

TEST_CASE("AudioResourceCache: acquireAsync same path returns same shared_ptr",
          "[audio][cache][async]") {
    ScopedTempDir tmp;
    auto path = writeWav(tmp.path(), "b.wav", 1, 22050, 0.2);

    AudioResourceCache cache;
    auto a = cache.acquireAsync(makeDesc(path));
    auto b = cache.acquireAsync(makeDesc(path));
    REQUIRE(a == b);

    (void)waitForCompletion(cache, path.string());
}

TEST_CASE(
    "AudioResourceCache: drainCompletions contains one entry per async job",
    "[audio][cache][async]") {
    ScopedTempDir tmp;
    auto p1 = writeWav(tmp.path(), "p1.wav", 1, 22050, 0.05);
    auto p2 = writeWav(tmp.path(), "p2.wav", 1, 22050, 0.05);

    AudioResourceCache cache;
    cache.acquireAsync(makeDesc(p1));
    cache.acquireAsync(makeDesc(p2));

    std::vector<std::string> seen;
    auto ok = pollUntil([&] {
        auto batch = cache.drainCompletions();
        for (auto& c : batch) seen.push_back(c.logicalPath);
        return seen.size() >= 2u;
    });
    REQUIRE(ok);
    std::sort(seen.begin(), seen.end());
    REQUIRE(seen.size() == 2u);
}

TEST_CASE("AudioResourceCache: evict removes from cache but keeps alive",
          "[audio][cache][evict]") {
    ScopedTempDir tmp;
    auto path = writeWav(tmp.path(), "ev.wav", 1, 22050, 0.05);

    AudioResourceCache cache;
    auto r = cache.acquire(makeDesc(path));
    REQUIRE(r.isOk());

    cache.evict(path.string());
    REQUIRE(cache.size() == 0u);

    REQUIRE(r.value()->isReady());
    REQUIRE(r.value()->frameCount() > 0u);
}

TEST_CASE("AudioResourceCache: clear empties the map",
          "[audio][cache][clear]") {
    ScopedTempDir tmp;
    auto path = writeWav(tmp.path(), "c.wav", 1, 22050, 0.05);
    AudioResourceCache cache;
    acquireForSideEffect(cache, makeDesc(path));
    REQUIRE(cache.size() == 1u);

    cache.clear();
    REQUIRE(cache.size() == 0u);
}

TEST_CASE("AudioResourceCache: listLogicalPaths reflects cache keys",
          "[audio][cache]") {
    ScopedTempDir tmp;
    auto p1 = writeWav(tmp.path(), "l1.wav", 1, 22050, 0.05);
    auto p2 = writeWav(tmp.path(), "l2.wav", 1, 22050, 0.05);

    AudioResourceCache cache;
    acquireForSideEffect(cache, makeDesc(p1));
    acquireForSideEffect(cache, makeDesc(p2));

    const auto paths = cache.listLogicalPaths();
    REQUIRE(paths.size() == 2u);
    std::vector<std::string> sorted = paths;
    std::sort(sorted.begin(), sorted.end());
    REQUIRE(sorted[0] == p1.string());
    REQUIRE(sorted[1] == p2.string());
}

TEST_CASE("AudioResourceCache: reload replaces cached resource",
          "[audio][cache][reload]") {
    ScopedTempDir tmp;
    auto path = writeWav(tmp.path(), "r.wav", 1, 22050, 0.05);

    AudioResourceCache cache;
    auto old = cache.acquire(makeDesc(path)).value();
    REQUIRE(old->isReady());

    auto reloaded = cache.reload(path.string());
    REQUIRE(reloaded.isOk());
    REQUIRE(reloaded.value()->isReady());
    REQUIRE(reloaded.value() != old);
    REQUIRE(old->isReady());
}

TEST_CASE("AudioResourceCache: reload unknown path -> ResourceNotFound",
          "[audio][cache][reload]") {
    AudioResourceCache cache;
    auto r = cache.reload("nope");
    REQUIRE(r.isErr());
    REQUIRE(r.error() == AudioError::ResourceNotFound);
}

TEST_CASE("AudioResourceCache: reloadAsync completes via drainCompletions",
          "[audio][cache][reload]") {
    ScopedTempDir tmp;
    auto path = writeWav(tmp.path(), "ra.wav", 1, 22050, 0.1);

    AudioResourceCache cache;
    acquireForSideEffect(cache, makeDesc(path));

    (void) cache.drainCompletions();

    REQUIRE(cache.reloadAsync(path.string()).isOk());

    auto comps = waitForCompletion(cache, path.string());
    REQUIRE(comps.size() == 1u);
    REQUIRE(comps[0].wasReload);
    REQUIRE(comps[0].resource->isReady());
}

TEST_CASE("AudioResourceCache: double reloadAsync fails with ResourceNotReady",
          "[audio][cache][reload]") {
    ScopedTempDir tmp;
    auto path = writeWav(tmp.path(), "dbl.wav", 1, 22050, 0.5);

    AudioResourceCache cache;
    acquireForSideEffect(cache, makeDesc(path));
    REQUIRE(cache.reloadAsync(path.string()).isOk());

    auto second = cache.reloadAsync(path.string());
    REQUIRE(second.isErr());
    REQUIRE(second.error() == AudioError::ResourceNotReady);

    (void)waitForCompletion(cache, path.string());
}

TEST_CASE(
    "AudioResourceCache: reloadAsync on unloaded path -> ResourceNotFound",
    "[audio][cache][reload]") {
    AudioResourceCache cache;
    auto r = cache.reloadAsync("missing");
    REQUIRE(r.isErr());
    REQUIRE(r.error() == AudioError::ResourceNotFound);
}

TEST_CASE("AudioResourceCache: needsReload false when mtime unchanged",
          "[audio][cache][mtime]") {
    ScopedTempDir tmp;
    auto path = writeWav(tmp.path(), "n.wav", 1, 22050, 0.05);

    AudioResourceCache cache;
    acquireForSideEffect(cache, makeDesc(path));
    REQUIRE_FALSE(cache.needsReload(path.string()));
}

TEST_CASE("AudioResourceCache: needsReload true after mtime bump",
          "[audio][cache][mtime]") {
    ScopedTempDir tmp;
    auto path = writeWav(tmp.path(), "nb.wav", 1, 22050, 0.05);

    AudioResourceCache cache;
    acquireForSideEffect(cache, makeDesc(path));

    std::error_code ec;
    const auto oldTime = std::filesystem::last_write_time(path, ec);
    if (ec) {
        WARN("cannot read mtime, skipping");
        return;
    }
    std::filesystem::last_write_time(path, oldTime + std::chrono::hours(1), ec);
    if (ec) {
        WARN("cannot set mtime, skipping");
        return;
    }
    REQUIRE(cache.needsReload(path.string()));
}

TEST_CASE("AudioResourceCache: needsReload false after file removal",
          "[audio][cache][mtime]") {
    ScopedTempDir tmp;
    auto path = writeWav(tmp.path(), "rm.wav", 1, 22050, 0.05);

    AudioResourceCache cache;
    acquireForSideEffect(cache, makeDesc(path));
    std::filesystem::remove(path);
    REQUIRE_FALSE(cache.needsReload(path.string()));
}

TEST_CASE("AudioResourceCache: workerCount clamps to 1 minimum",
          "[audio][cache][workers]") {
    AudioResourceCache cache;
    cache.setWorkerCount(0);
    REQUIRE(cache.workerCount() == 1u);
    cache.setWorkerCount(4);
    REQUIRE(cache.workerCount() == 4u);
}

TEST_CASE("AudioResourceCache: acquire pushes a synthetic completion (current)",
          "[audio][cache][sync]") {
    ScopedTempDir tmp;
    auto path = writeWav(tmp.path(), "syn.wav", 1, 22050, 0.05);

    AudioResourceCache cache;
    REQUIRE(cache.acquire(makeDesc(path)).isOk());

    auto comps = cache.drainCompletions();
    REQUIRE(comps.size() == 1u);
    REQUIRE_FALSE(comps[0].wasReload);
    REQUIRE(comps[0].resource->isReady());
}

TEST_CASE("AudioResourceCache: evict between enqueue and completion is safe",
          "[audio][cache][evict][async]") {
    ScopedTempDir tmp;
    auto path = writeWav(tmp.path(), "evx.wav", 1, 44100, 0.2);

    AudioResourceCache cache;
    cache.acquireAsync(makeDesc(path));
    cache.evict(path.string());

    auto observed = pollUntil(
        [&] {
            auto batch = cache.drainCompletions();
            return !batch.empty();
        },
        std::chrono::seconds(2));

    REQUIRE_FALSE(observed);
}