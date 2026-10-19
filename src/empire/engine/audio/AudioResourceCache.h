#pragma once

#include <empire/engine/audio/AudioResource.h>

#include <condition_variable>
#include <cstddef>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>

#include "AudioError.h"

namespace empire::audio {

// Engine-independent. Owns strong refs to loaded resources. Surives device
// migration. Thread-unsafe(main-thread only).
class AudioResourceCache {
public:
    AudioResourceCache();
    ~AudioResourceCache();

    AudioResourceCache(const AudioResourceCache&) = delete;
    AudioResourceCache& operator=(const AudioResourceCache&) = delete;

    // Returns caached entry or loads a new one. Failure leaves the cache
    // untouched.
    AudioResult<std::shared_ptr<const AudioResource>> acquire(
        const AudioResourceDesc& desc);

    std::shared_ptr<const AudioResource> acquireAsync(
        const AudioResourceDesc& desc);
    // Force reload of a cached entry. The old resource stays alive as long as
    // someone else holds a shared_ptr to it.
    AudioResult<std::shared_ptr<const AudioResource>> reload(
        std::string_view logicalPath);
    AudioResult<void> reloadAsync(std::string_view logicalPath);

    void evict(std::string_view logicalPath);
    void clear();

    std::size_t size() const noexcept {
        return m_cache.size();
    }
    std::vector<std::string> listLogicalPaths() const;

    // True if the file on disk has a newer mtime than the cached entry. Never
    // throws.
    bool needsReload(std::string_view logicalPath) const;

    void setWorkerCount(std::size_t n);
    std::size_t workerCount() const noexcept;

    struct CompletedLoad {
        std::string logicalPath;
        std::shared_ptr<const AudioResource> resource;
        bool wasReload = false;
    };
    std::vector<AudioResourceCache::CompletedLoad> drainCompletions();

private:
    struct Job {
        std::shared_ptr<AudioResource> resource;
        AudioResourceDesc desc;
        bool isReload = false;
    };
    struct Entry {
        std::shared_ptr<const AudioResource> resource;
        std::shared_ptr<const AudioResource> pendingReload;
        AudioResourceDesc desc;
    };

    void startWorkers(std::size_t n);
    void workerLoop();
    void stopWorkers() noexcept;
    void enqueue(Job job);

    std::unordered_map<std::string, Entry> m_cache;

    std::vector<std::thread> m_workers;
    std::size_t m_workerTarget = 0;
    std::mutex m_jobMutex;
    std::condition_variable m_cv;
    std::queue<Job> m_jobs;
    bool m_quit = false;

    mutable std::mutex m_completionMutex;
    std::vector<Job> m_completedJobs;
};
}  // namespace empire::audio