#include <empire/engine/audio/AudioChannels.h>
#include <empire/engine/audio/AudioResourceCache.h>
#include <empire/engine/log/Logger.h>

#include <filesystem>
#include <system_error>
#include <utility>

namespace empire::audio {

using ResPtr = std::shared_ptr<const AudioResource>;
using ResRes = AudioResult<ResPtr>;

AudioResourceCache::AudioResourceCache() {
    const unsigned hw = std::thread::hardware_concurrency();
    const std::size_t n =
        (hw > 1) ? static_cast<std::size_t>(std::min(4u, hw - 1)) : 1u;
    startWorkers(n);
}

AudioResourceCache::~AudioResourceCache() {
    stopWorkers();
}

AudioResult<std::shared_ptr<const AudioResource>> AudioResourceCache::acquire(
    const AudioResourceDesc& desc) {
    if (desc.path.empty()) {
        return ResRes::err(AudioError::InvalidArgument);
    }
    if (auto it = m_cache.find(desc.path); it != m_cache.end()) {
        auto& entry = it->second;
        if (entry.resource && entry.resource->isReady()) {
            return ResRes::ok(entry.resource);
        }
    }

    auto res = std::make_shared<AudioResource>();
    auto r = res->load(desc);
    if (!r.isOk()) {
        return ResRes::err(r.error());
    }

    Entry e;
    e.resource = res;
    e.desc = desc;
    std::shared_ptr<const AudioResource> stored = res;
    m_cache[desc.path] = std::move(e);

    Job synthetic;
    synthetic.resource = res;
    synthetic.desc = desc;
    synthetic.isReload = false;
    {
        std::lock_guard g(m_completionMutex);
        m_completedJobs.push_back(std::move(synthetic));
    }

    return ResRes::ok(std::move(stored));
}

std::shared_ptr<const AudioResource> AudioResourceCache::acquireAsync(
    const AudioResourceDesc& desc) {
    if (desc.path.empty()) {
        return nullptr;
    }

    if (auto it = m_cache.find(desc.path); it != m_cache.end()) {
        auto state = it->second.resource->state();
        if (state == ResourceState::Ready || state == ResourceState::Loading) {
            return it->second.resource;
        }
    }

    auto res = std::make_shared<AudioResource>();
    res->markLoading();

    Entry e;
    e.resource = res;
    e.desc = desc;
    std::shared_ptr<const AudioResource> out = res;
    m_cache[desc.path] = std::move(e);

    enqueue({res, desc, false});
    return out;
}

AudioResult<std::shared_ptr<const AudioResource>> AudioResourceCache::reload(
    std::string_view logicalPath) {
    const std::string key(logicalPath);
    auto it = m_cache.find(key);
    if (it == m_cache.end()) {
        return ResRes::err(AudioError::ResourceNotFound);
    }

    const AudioResourceDesc desc = it->second.desc;
    auto res = std::make_shared<AudioResource>();
    auto r = res->load(desc);
    if (!r.isOk()) {
        return ResRes::err(r.error());
    }

    GAME_LOG_CH_INFO(channels::kResource, "reloaded resource '{}'", key);
    it->second.resource = std::move(res);
    return ResRes::ok(it->second.resource);
}

AudioResult<void> AudioResourceCache::reloadAsync(
    std::string_view logicalPath) {
    const std::string key(logicalPath);
    auto it = m_cache.find(key);
    if (it == m_cache.end()) {
        return AudioResult<void>::err(AudioError::ResourceNotFound);
    }
    auto& entry = it->second;
    if (!entry.resource || entry.resource->isLoading()) {
        return AudioResult<void>::err(AudioError::ResourceNotReady);
    }
    if (entry.pendingReload) {
        return AudioResult<void>::err(AudioError::ResourceNotReady);
    }

    auto fresh = std::make_shared<AudioResource>();
    fresh->markLoading();
    entry.pendingReload = fresh;

    enqueue({fresh, entry.desc, /*isReload=*/true});
    return AudioResult<void>::ok();
}

void AudioResourceCache::evict(std::string_view logicalPath) {
    m_cache.erase(std::string(logicalPath));
}

void AudioResourceCache::clear() {
    m_cache.clear();
}

std::vector<AudioResourceCache::CompletedLoad>
AudioResourceCache::drainCompletions() {
    std::vector<Job> jobs;
    {
        std::lock_guard g(m_completionMutex);
        jobs.swap(m_completedJobs);
    }

    std::vector<CompletedLoad> out;
    out.reserve(jobs.size());
    for (auto& job : jobs) {
        auto it = m_cache.find(job.desc.path);
        if (it == m_cache.end()) {
            continue;
        }
        auto& entry = it->second;

        if (job.isReload) {
            if (entry.pendingReload.get() != job.resource.get()) {
                continue;
            }
            if (job.resource->isReady()) {
                entry.resource = job.resource;
            }
            entry.pendingReload.reset();
        } else {
            if (entry.resource.get() != job.resource.get()) {
                continue;
            }
        }
        out.push_back(CompletedLoad{
            job.desc.path,
            std::shared_ptr<const AudioResource>(job.resource),
            job.isReload,
        });
    }
    return out;
}

std::vector<std::string> AudioResourceCache::listLogicalPaths() const {
    std::vector<std::string> out;
    out.reserve(m_cache.size());
    for (const auto& [k, _] : m_cache) {
        out.push_back(k);
    }
    return out;
}

bool AudioResourceCache::needsReload(std::string_view logicalPath) const {
    const std::string key(logicalPath);
    auto it = m_cache.find(key);
    if (it == m_cache.end() || !it->second.resource) {
        return false;
    }
    const auto& res = *it->second.resource;
    if (res.resolvedPath().empty()) {
        return false;
    }
    std::error_code ec;
    const auto current =
        std::filesystem::last_write_time(res.resolvedPath(), ec);
    if (ec) {
        return false;
    }
    return current != res.mtime();
}

void AudioResourceCache::setWorkerCount(std::size_t n) {
    if (n == 0) {
        n = 1;
    }
    if (n == m_workerTarget) {
        return;
    }
    stopWorkers();
    startWorkers(n);
}

std::size_t AudioResourceCache::workerCount() const noexcept {
    return m_workerTarget;
}

void AudioResourceCache::startWorkers(std::size_t n) {
    if (n == 0) {
        n = 1;
    }
    m_workerTarget = n;
    m_workers.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        m_workers.emplace_back(&AudioResourceCache::workerLoop, this);
    }
}

void AudioResourceCache::workerLoop() {
    for (;;) {
        Job job;
        {
            std::unique_lock lk(m_jobMutex);
            m_cv.wait(lk, [&] { return m_quit || !m_jobs.empty(); });
            if (m_quit && m_jobs.empty()) {
                return;
            }
            job = std::move(m_jobs.front());
            m_jobs.pop();
        }

        (void)job.resource->loadInBackground(job.desc);
        {
            std::lock_guard g(m_completionMutex);
            m_completedJobs.push_back(std::move(job));
        }
    }
}

void AudioResourceCache::stopWorkers() noexcept {
    {
        std::lock_guard g(m_jobMutex);
        m_quit = true;
    }
    m_cv.notify_all();
    for (auto& t : m_workers) {
        if (t.joinable()) {
            t.join();
        }
    }
    m_workers.clear();
    {
        std::lock_guard g(m_jobMutex);
        m_quit = false;
    }
    m_workerTarget = 0;
}

void AudioResourceCache::enqueue(Job job) {
    {
        std::lock_guard g(m_jobMutex);
        m_jobs.push(std::move(job));
    }
    m_cv.notify_one();
}

}  // namespace empire::audio