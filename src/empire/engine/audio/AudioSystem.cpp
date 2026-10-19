#include <empire/engine/audio/AudioChannels.h>
#include <empire/engine/audio/AudioMusicPlayer.h>
#include <empire/engine/audio/AudioResourceCache.h>
#include <empire/engine/audio/AudioSystem.h>
#include <empire/engine/audio/detail/MaAudioBufferRef.h>
#include <empire/engine/audio/detail/MaRaii.h>
#include <empire/engine/audio/detail/MaResult.h>
#include <empire/engine/log/Logger.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>
#include <random>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace empire::audio {
namespace {
thread_local std::mt19937 g_rng{std::random_device{}()};

float randomizedPitch(float basePitch, float variation) {
    if (variation <= 0.0f) {
        return basePitch;
    }
    std::uniform_real_distribution<float> d(-variation, variation);
    return basePitch * (1.0f + d(g_rng));
}
constexpr float kVolumeEpsilon = 1.0e-4f;
constexpr float kHotReloadPollInterval = 1.0f;
}  // namespace

// Impl
struct AudioSystem::Impl {
    AudioEventLog eventLog;
    std::unordered_map<std::string, std::vector<SoundId>> pendingByPath;
    AudioSystem::SoundReadyCallback soundReadyCb;

    AudioDeviceManager device;
    AudioResourceCache resourceCache;
    AudioBusRegistry buses;
    AudioVoicePool voices;
    AudioMusicPlayer musicPlayer;
    Listener3D listener3D{};
    bool listener3DSet = false;

    // Voice. Holds shared_ptr to its own resource so the underlying PCM
    // survives hot-reload and migration for the lifetime of the voice.

    struct SoundEntry {
        std::string name;
        SoundDesc desc;
        std::shared_ptr<const AudioResource> resource;  // current payload
        std::uint32_t groupKey = 0;
    };

    util::HandleTable<SoundTag, SoundEntry> soundRegistry;
    std::unordered_map<std::string, SoundId> nameToId;

    std::uint32_t nextGroupKey = 1;

    // Volumes/state
    float masterV = 1.0f;
    float sfxV = 1.0f;
    float musicV = 1.0f;
    bool isPaused = false;
    // Hot-reload
    bool autoHotReload = false;
    float hotReloadPollAccum = 0.0f;

    // Helpers

    bool ready() const noexcept {
        return device.ready();
    }
    // music

    detail::MaSoundGroup* resolveGroup(std::string_view want) {
        BusHandle h = buses.find(want);
        if (!h.isValid()) {
            if (want == "master") {
                GAME_LOG_CH_ERROR(channels::kBus,
                                  "bus '{}' not found - audio cannot play",
                                  want);
                return nullptr;
            }
            GAME_LOG_CH_WARN(channels::kBus,
                             "bus '{}' not found, falling back to 'master'",
                             want);

            h = buses.find("master");
            if (!h.isValid()) {
                GAME_LOG_CH_ERROR(channels::kBus,
                                  "fallback bus 'master' not found");
                return nullptr;
            }
        }
        return buses.group(h);
    }

    // load, reload, unload

    AudioResult<SoundId> loadSoundImpl(std::string_view name,
                                       const SoundDesc& desc, bool asyncLoad) {
        using Res = AudioResult<SoundId>;
        if (name.empty()) {
            return Res::err(AudioError::InvalidArgument);
        }

        const std::string key(name);

        if (auto it = nameToId.find(key); it != nameToId.end()) {
            if (soundRegistry.isValid(it->second)) {
                auto* e = soundRegistry.get(it->second);
                if (e && e->resource &&
                    (e->resource->isReady() || e->resource->isLoading())) {
                    if (e->desc.resource.path != desc.resource.path) {
                        GAME_LOG_CH_WARN(
                            channels::kResource,
                            "loadSound('{}'): already loaded with path '{}' "
                            "(requested '{}'); use unloadSound() first",
                            key, e->desc.resource.path, desc.resource.path);
                        return Res::err(AudioError::ResourceAlreadyLoaded);
                    }
                    return Res::ok(it->second);
                }
                soundRegistry.erase(it->second);
                nameToId.erase(it);
            } else {
                nameToId.erase(it);
            }
        }

        std::shared_ptr<const AudioResource> res;
        if (asyncLoad) {
            res = resourceCache.acquireAsync(desc.resource);
            if (!res) {
                return Res::err(AudioError::InvalidArgument);
            }
        } else {
            auto r = resourceCache.acquire(desc.resource);
            if (!r.isOk()) {
                eventLog.push(AudioEventKind::ResourceLoadFailed, r.error(),
                              AudioEventLog::hashStr(desc.resource.path));
                return Res::err(r.error());
            }
            res = r.value();
        }

        SoundEntry e;
        e.name = key;
        e.desc = desc;
        e.resource = std::move(res);
        e.groupKey = nextGroupKey++;
        if (nextGroupKey == 0) {
            nextGroupKey = 1;
        }

        const SoundId id = soundRegistry.insert(std::move(e));
        nameToId[key] = id;
        SoundEntry* stored = soundRegistry.get(id);
        if (asyncLoad && stored && stored->resource &&
            stored->resource->isLoading()) {
            pendingByPath[desc.resource.path].push_back(id);
        }
        return Res::ok(id);
    }

    AudioResult<void> reloadSoundImpl(std::string_view name) {
        auto it = nameToId.find(std::string(name));
        if (it == nameToId.end()) {
            return AudioResult<void>::err(AudioError::ResourceNotFound);
        }
        SoundEntry* e = soundRegistry.get(it->second);
        if (e == nullptr) {
            return AudioResult<void>::err(AudioError::ResourceNotFound);
        }

        auto r = resourceCache.reload(e->resource->logicalPath());
        if (!r.isOk()) {
            return AudioResult<void>::err(r.error());
        }
        e->resource = r.value();
        return AudioResult<void>::ok();
    }

    void runHotReloadCheck() {
        std::vector<std::string> toReload;
        soundRegistry.forEach([&](SoundId, SoundEntry& e) {
            if (resourceCache.needsReload(e.desc.resource.path)) {
                toReload.push_back(e.name);
            }
        });
        for (const auto& n : toReload) {
            GAME_LOG_CH_DEBUG(channels::kResource, "auto-reload '{}'", n);
            (void)reloadSoundImpl(n);
        }
    }

    // Migration hooks

    void teardownEngineDependents() {
        musicPlayer.onBeforeMigration();
        voices.uninit();
        buses.uninit();
    }

    void rebuildEngineDependents() {
        ma_engine* engine = device.engine();
        if (engine == nullptr) {
            return;
        }

        auto rb = buses.init(engine);
        if (!rb.isOk()) {
            GAME_LOG_CH_ERROR(channels::kBus, "bus init failed: {}",
                              describe(rb.error()));
            return;
        }

        auto rv = voices.init(engine);
        if (!rv.isOk()) {
            GAME_LOG_CH_ERROR(channels::kEngine, "voice pool init failed: {}",
                              describe(rv.error()));
            return;
        }
        if (listener3DSet) {
            ma_engine_listener_set_position(engine, 0, listener3D.px,
                                            listener3D.py, listener3D.pz);
            ma_engine_listener_set_direction(engine, 0, listener3D.fx,
                                             listener3D.fy, listener3D.fz);
            ma_engine_listener_set_world_up(engine, 0, listener3D.ux,
                                            listener3D.uy, listener3D.uz);
        }
        musicPlayer.onAfterMigration();

        if (!isPaused) {
            const ma_result sr = ma_engine_start(engine);
            if (sr != MA_SUCCESS) {
                GAME_LOG_CH_ERROR(channels::kEngine,
                                  "ma_engine_start failed: {}",
                                  detail::maResultName(sr));
            }
        }
    }

    VoiceHandle startVoice(SoundEntry* e, float volume, float pitch, float pan,
                           const std::optional<Spatial2D>& s2D,
                           const std::optional<Spatial3D>& s3D) {
        if (e == nullptr || !e->resource || !e->resource->isReady())
            return kInvalidVoice;

        const std::string_view want = e->desc.bus.empty()
                                          ? std::string_view{"sfx"}
                                          : std::string_view{e->desc.bus};
        detail::MaSoundGroup* group = resolveGroup(want);
        if (group == nullptr) return kInvalidVoice;

        VoiceStartDesc vd;
        vd.resource = e->resource;
        vd.group = group;
        vd.volume = volume * e->desc.baseVolume;
        vd.pitch =
            randomizedPitch(pitch * e->desc.basePitch, e->desc.pitchVariation);
        vd.pan = pan;
        vd.loop = e->desc.loop;
        vd.priority = e->desc.priority;
        vd.maxConcurrent = e->desc.maxConcurrent;
        vd.soundGroupKey = e->groupKey;
        vd.spatial2D = s2D;
        vd.spatial3D = s3D;
        return voices.start(vd).handle;
    }
};

// AudioSystem

AudioSystem::AudioSystem() : m_impl(std::make_unique<Impl>()) {
}

AudioSystem::~AudioSystem() {
    shutdown();
}

bool AudioSystem::init() {
    if (!m_impl) {
        return false;
    }
    if (auto* logger = log::defaultLogger()) {
        channels::registerDefaults(*logger);
    }

    auto r = m_impl->buses.initDefaults();
    if (!r.isOk()) {
        GAME_LOG_CH_ERROR(channels::kBus, "bus defaults failed: {}",
                          describe(r.error()));
        return false;
    }

    m_impl->voices.setEventLog(&m_impl->eventLog);
    m_impl->device.setEventLog(&m_impl->eventLog);
    m_impl->musicPlayer.setEventLog(&m_impl->eventLog);
    m_impl->musicPlayer.init(m_impl->device, m_impl->buses);
    m_impl->device.setMigrationFadeDurations(0.3f, 0.2f);
    m_impl->device.setRetryBackoff(1.0f, 30.0f);
    m_impl->device.setBeforeMigration(
        [this]() { m_impl->teardownEngineDependents(); });
    m_impl->device.setAfterMigration(
        [this]() { m_impl->rebuildEngineDependents(); });

    m_impl->device.setMigrationCallback([](const MigrationEvent& ev) {
        // extra hook point
        if (!ev.success) {
            GAME_LOG_CH_WARN(channels::kDevice,
                             "miigration callback: failed (reason={}, old={}",
                             static_cast<int>(ev.reason), ev.oldDevice);
        }
    });

    const auto rd = m_impl->device.init();
    if (!rd.isOk()) {
        GAME_LOG_CH_ERROR(channels::kDevice, "audio init failed: {}",
                          describe(rd.error()));
        return false;
    }
    return true;
}

void AudioSystem::shutdown() {
    if (!m_impl) {
        return;
    }
    m_impl->musicPlayer.uninit();
    m_impl->teardownEngineDependents();
    m_impl->device.shutdown();
    m_impl->soundRegistry.clear();
    m_impl->nameToId.clear();
    m_impl->resourceCache.clear();
}

bool AudioSystem::ready() const noexcept {
    return m_impl && m_impl->ready();
}

// sfx

AudioResult<SoundId> AudioSystem::loadSound(std::string_view name,
                                            const SoundDesc& desc) {
    if (!m_impl) {
        return AudioResult<SoundId>::err(AudioError::NotInitialized);
    }
    return m_impl->loadSoundImpl(name, desc, false);
}

AudioResult<SoundId> AudioSystem::loadSoundAsync(std::string_view name,
                                                 const SoundDesc& desc) {
    if (!m_impl) {
        return AudioResult<SoundId>::err(AudioError::NotInitialized);
    }
    return m_impl->loadSoundImpl(name, desc, /*asyncLoad=*/true);
}

AudioResult<void> AudioSystem::reloadSoundAsync(std::string_view name) {
    if (!m_impl) {
        return AudioResult<void>::err(AudioError::NotInitialized);
    }
    auto& I = *m_impl;
    auto it = I.nameToId.find(std::string(name));
    if (it == I.nameToId.end()) {
        return AudioResult<void>::err(AudioError::ResourceNotFound);
    }
    Impl::SoundEntry* e = I.soundRegistry.get(it->second);
    if (e == nullptr || !e->resource) {
        return AudioResult<void>::err(AudioError::ResourceNotFound);
    }
    return I.resourceCache.reloadAsync(e->resource->logicalPath());
}

AudioResult<void> AudioSystem::reloadSound(std::string_view name) {
    if (!m_impl) {
        return AudioResult<void>::err(AudioError::NotInitialized);
    }
    return m_impl->reloadSoundImpl(name);
}

AudioResult<void> AudioSystem::reloadAllSounds() {
    if (!m_impl) {
        return AudioResult<void>::err(AudioError::NotInitialized);
    }
    std::vector<std::string> names;
    m_impl->soundRegistry.forEach(
        [&](SoundId, Impl::SoundEntry& e) { names.push_back(e.name); });
    AudioResult<void> last = AudioResult<void>::ok();
    for (const auto& n : names) {
        auto r = m_impl->reloadSoundImpl(n);
        if (!r.isOk()) {
            last = std::move(r);
        }
    }
    return last;
}

void AudioSystem::unloadSound(std::string_view name) {
    if (!m_impl) {
        return;
    }
    auto& I = *m_impl;
    const std::string key(name);
    auto it = I.nameToId.find(key);
    if (it == I.nameToId.end()) {
        return;
    }

    const SoundId id = it->second;
    Impl::SoundEntry* e = I.soundRegistry.get(id);
    const std::uint32_t groupKey = e ? e->groupKey : 0;
    std::string path;
    if (e && e->resource) {
        path = e->resource->logicalPath();
    }

    I.soundRegistry.erase(it->second);
    I.nameToId.erase(it);
    if (!path.empty()) {
        auto pit = I.pendingByPath.find(path);
        if (pit != I.pendingByPath.end()) {
            auto& v = pit->second;
            v.erase(std::remove(v.begin(), v.end(), id), v.end());
            if (v.empty()) {
                I.pendingByPath.erase(pit);
            }
        }
    }
    if (groupKey) {
        I.voices.stopByGroupKey(groupKey);
    }
}

void AudioSystem::unloadAllSounds() {
    if (!m_impl) {
        return;
    }
    auto& I = *m_impl;
    std::vector<std::uint32_t> keys;
    keys.reserve(I.soundRegistry.size());
    I.soundRegistry.forEach(
        [&](SoundId, Impl::SoundEntry& e) { keys.push_back(e.groupKey); });
    I.soundRegistry.clear();
    I.nameToId.clear();
    I.pendingByPath.clear();
    for (auto k : keys) {
        I.voices.stopByGroupKey(k);
    }
}

// Cache

void AudioSystem::evictResource(std::string_view logicalPath) {
    if (m_impl) {
        m_impl->resourceCache.evict(logicalPath);
    }
}

void empire::audio::AudioSystem::clearResourceCache() {
    if (m_impl) {
        m_impl->resourceCache.clear();
    }
}

std::size_t AudioSystem::cachedResourceCount() const noexcept {
    return m_impl ? m_impl->resourceCache.size() : 0;
}

void AudioSystem::setAutoHotReloadEnabled(bool enabled) noexcept {
    if (m_impl) {
        m_impl->autoHotReload = enabled;
    }
}

bool AudioSystem::autoHotReloadEnabled() const noexcept {
    return m_impl && m_impl->autoHotReload;
}

// Playback

VoiceHandle AudioSystem::play(SoundId id, float volume, float pitch,
                              float panX) {
    if (!m_impl) {
        return kInvalidVoice;
    }
    auto* e = m_impl->soundRegistry.get(id);
    return m_impl->startVoice(e, volume, pitch, panX, std::nullopt,
                              std::nullopt);
}

VoiceHandle AudioSystem::playAt2D(SoundId id, float x, float y, float volume,
                                  float pitch) {
    if (!m_impl) {
        return kInvalidVoice;
    }
    auto* e = m_impl->soundRegistry.get(id);
    return m_impl->startVoice(e, volume, pitch, 0.0f, Spatial2D{x, y},
                              std::nullopt);
}

VoiceHandle AudioSystem::playAt3D(SoundId id, const Spatial3D& spatial,
                                  float volume, float pitch) {
    if (!m_impl) {
        return kInvalidVoice;
    }
    auto* e = m_impl->soundRegistry.get(id);
    return m_impl->startVoice(e, volume, pitch, 0.0f, std::nullopt, spatial);
}

void AudioSystem::setListener2D(const Listener2D& l) {
    if (m_impl) {
        m_impl->voices.setListener2D(l);
    }
}

const Listener2D& AudioSystem::listener2D() const noexcept {
    static const Listener2D kDefault{};
    return m_impl ? m_impl->voices.listener2D() : kDefault;
}

void AudioSystem::setListener3D(const Listener3D& l) {
    if (!m_impl) {
        return;
    }
    m_impl->listener3D = l;
    m_impl->listener3DSet = true;

    ma_engine* eng = m_impl->device.engine();
    if (eng == nullptr) {
        return;
    }
    ma_engine_listener_set_position(eng, 0, l.px, l.py, l.pz);
    ma_engine_listener_set_direction(eng, 0, l.fx, l.fy, l.fz);
    ma_engine_listener_set_world_up(eng, 0, l.ux, l.uy, l.uz);
}
const Listener3D& AudioSystem::listener3D() const noexcept {
    static const Listener3D kDefault{};
    return m_impl ? m_impl->listener3D : kDefault;
}

void AudioSystem::stopVoice(VoiceHandle h) {
    if (m_impl) {
        m_impl->voices.stop(h);
    }
}

bool AudioSystem::isVoicePlaying(VoiceHandle h) const noexcept {
    return m_impl && m_impl->voices.isPlaying(h);
}

void AudioSystem::setVoiceFinishedCallback(VoiceFinishedCallback fn) {
    if (m_impl) {
        m_impl->voices.setFinishedCallback(std::move(fn));
    }
}

std::size_t AudioSystem::activeVoiceCount() const noexcept {
    return m_impl ? m_impl->voices.activeCount() : 0;
}

std::size_t AudioSystem::stolenVoiceCount() const noexcept {
    return m_impl ? m_impl->voices.stolenTotal() : 0;
}

// Music
void AudioSystem::playMusic(std::string_view path, float fadeInSec, bool loop) {
    if (m_impl) {
        m_impl->musicPlayer.play(path, fadeInSec, loop);
    }
}

void AudioSystem::stopMusic(float fadeOutSec) {
    if (m_impl) {
        m_impl->musicPlayer.stop(fadeOutSec);
    }
}

void AudioSystem::crossfadeTo(std::string_view path, float durationSec,
                              bool loop) {
    if (m_impl) {
        m_impl->musicPlayer.crossfadeTo(path, durationSec, loop);
    }
}

// buses

AudioBusRegistry& AudioSystem::buses() noexcept {
    return m_impl->buses;
}
const AudioBusRegistry& AudioSystem::buses() const noexcept {
    return m_impl->buses;
}

// volume

void AudioSystem::setMasterVolume(float v) {
    if (m_impl) {
        m_impl->buses.setVolume(m_impl->buses.find("master"), v);
    }
}

void AudioSystem::setSfxVolume(float v) {
    if (m_impl) {
        m_impl->buses.setVolume(m_impl->buses.find("sfx"), v);
    }
}

void AudioSystem::setMusicVolume(float v) {
    if (m_impl) {
        m_impl->buses.setVolume(m_impl->buses.find("music"), v);
    }
}

float AudioSystem::masterVolume() const noexcept {
    return m_impl ? m_impl->buses.volume(m_impl->buses.find("master")) : 0.0f;
}

float AudioSystem::sfxVolume() const noexcept {
    return m_impl ? m_impl->buses.volume(m_impl->buses.find("sfx")) : 0.0f;
}

float AudioSystem::musicVolume() const noexcept {
    return m_impl ? m_impl->buses.volume(m_impl->buses.find("music")) : 0.0f;
}

void AudioSystem::setBusVolume(std::string_view busName, float v) {
    if (m_impl) {
        m_impl->buses.setVolume(m_impl->buses.find(busName), v);
    }
}

float AudioSystem::busVolume(std::string_view busName) const noexcept {
    return m_impl ? m_impl->buses.volume(m_impl->buses.find(busName)) : 0.0f;
}

void AudioSystem::applyBusSnapshot(const AudioBusSnapshot& snap,
                                   float fadeSec) {
    if (m_impl) {
        m_impl->buses.applySnapshot(snap, fadeSec);
    }
}

// per-frame
void AudioSystem::update(float dt) {
    if (!m_impl) {
        return;
    }
    auto& I = *m_impl;
    I.device.update(dt);
    if (ma_engine* eng = I.device.engine()) {
        ma_engine_set_volume(eng, I.device.migrationGain());
    }

    for (auto& c : I.resourceCache.drainCompletions()) {
        if (c.wasReload) {
            GAME_LOG_CH_INFO(channels::kResource, "async reload comple '{}'",
                             c.logicalPath);
            continue;
        }

        auto pit = I.pendingByPath.find(c.logicalPath);
        if (pit == I.pendingByPath.end()) {
            continue;
        }
        auto waiters = std::move(pit->second);
        I.pendingByPath.erase(pit);

        AudioResult<void> result =
            (c.resource && c.resource->isReady())
                ? AudioResult<void>::ok()
                : AudioResult<void>::err(AudioError::DecodeFailed);
        if (!result.isOk()) {
            I.eventLog.push(AudioEventKind::ResourceLoadFailed, result.error(),
                            AudioEventLog::hashStr(c.logicalPath));
        }
        if (I.soundReadyCb) {
            for (SoundId id : waiters) {
                if (!I.soundRegistry.isValid(id)) {
                    continue;
                }
                I.soundReadyCb(id, result);
            }
        }
    }

    I.buses.update(dt);
    I.voices.update(dt);
    I.musicPlayer.update(dt);

    if (I.autoHotReload) {
        I.hotReloadPollAccum += dt;
        if (I.hotReloadPollAccum >= kHotReloadPollInterval) {
            I.hotReloadPollAccum = 0.0f;
            I.runHotReloadCheck();
        }
    }
}

void AudioSystem::setPaused(bool paused) {
    if (!m_impl || !m_impl->ready()) {
        return;
    }
    if (paused == m_impl->isPaused) {
        return;
    }

    m_impl->isPaused = paused;

    ma_engine* engine = m_impl->device.engine();
    if (engine == nullptr) {
        return;
    }
    if (paused) {
        ma_engine_stop(engine);
    } else {
        ma_engine_start(engine);
    }
}

bool AudioSystem::paused() const noexcept {
    return m_impl && m_impl->isPaused;
}

// Devices

const std::vector<AudioDeviceInfo>& AudioSystem::listOutputDevices()
    const noexcept {
    static const std::vector<AudioDeviceInfo> kEmpty{};
    return m_impl ? m_impl->device.listOutputDevices() : kEmpty;
}

int AudioSystem::currentOutputDevice() const noexcept {
    return m_impl ? m_impl->device.currentDevice() : -1;
}

bool AudioSystem::setOutputDevice(int index) {
    if (!m_impl) {
        return false;
    }
    const auto r = m_impl->device.setOutputDevice(index);
    if (!r.isOk()) {
        GAME_LOG_CH_WARN(channels::kDevice, "setOutputDevice({}) failed: {}",
                         index, describe(r.error()));
        return false;
    }
    return true;  // request accepted; completion via MigrationCallback
}

AudioStats AudioSystem::stats() const {
    AudioStats s;
    if (!m_impl) return s;
    const auto& I = *m_impl;
    s.deviceReady = I.device.ready();
    s.currentDeviceIndex = I.device.currentDevice();
    s.followsSystemDefault = I.device.followsSystemDefault();
    s.isMigrating = I.device.isMigrating();
    s.migrationGain = I.device.migrationGain();
    s.voicesActive = I.voices.activeCount();
    s.voicesCapacity = AudioVoicePool::capacity();
    s.voicesStolenTotal = I.voices.stolenTotal();
    s.busCount = I.buses.size();
    s.anySolo = I.buses.anySolo();
    s.cachedResources = I.resourceCache.size();
    std::size_t pending = 0;
    for (const auto& [_, ids] : I.pendingByPath) {
        pending += ids.size();
    }
    s.queuedAsyncJobs = pending;
    s.musicPlaying = I.musicPlayer.isPlaying();
    s.musicPaused = I.musicPlayer.isPaused();
    s.musicQueueSize = I.musicPlayer.queuedCount();
    s.eventTotal = I.eventLog.totalCount();
    return s;
}

std::string AudioSystem::diagnosticsJson() const {
    if (!m_impl) return "{}";
    const auto& I = *m_impl;
    const AudioStats s = stats();
    nlohmann::json j;

    j["device"]["ready"] = s.deviceReady;
    j["device"]["currentIndex"] = s.currentDeviceIndex;
    j["device"]["followsDefault"] = s.followsSystemDefault;
    j["device"]["isMigrating"] = s.isMigrating;
    j["device"]["migrationGain"] = s.migrationGain;
    for (const auto& d : I.device.listOutputDevices()) {
        j["device"]["devices"].push_back(
            {{"name", d.name}, {"isDefault", d.isDefault}});
    }

    j["voices"]["active"] = s.voicesActive;
    j["voices"]["capacity"] = s.voicesCapacity;
    j["voices"]["stolenTotal"] = s.voicesStolenTotal;

    j["buses"]["count"] = s.busCount;
    j["buses"]["anySolo"] = s.anySolo;
    I.buses.forEach([&](BusHandle, const AudioBus& b) {
        j["buses"]["entries"].push_back({
            {"name", b.name},
            {"volume", b.volume},
            {"muted", b.muted},
            {"solo", b.solo},
            {"soloKill", b.soloKill},
        });
    });

    j["resources"]["cached"] = s.cachedResources;
    j["resources"]["queueAsync"] = s.queuedAsyncJobs;

    j["music"]["playing"] = s.musicPlaying;
    j["music"]["paused"] = s.musicPaused;
    j["music"]["queueSize"] = s.musicQueueSize;
    j["music"]["currentPath"] = I.musicPlayer.currentPath();

    j["events"]["total"] = I.eventLog.totalCount();
    for (const auto& e : I.eventLog.snapshot()) {
        j["events"]["recent"].push_back({
            {"tick", e.tick},
            {"kind", static_cast<int>(e.kind)},
            {"error", describe(e.error)},
            {"a", e.a},
            {"b", e.b},
        });
    }
    return j.dump(2);
}

bool AudioSystem::dumpDiagnostics(std::string_view path) const {
    std::ofstream f(std::string(path), std::ios::trunc);
    if (!f) return false;
    f << diagnosticsJson();
    return f.good();
}

void AudioSystem::setBusMuted(std::string_view name, bool muted) {
    if (m_impl) m_impl->buses.setMuted(m_impl->buses.find(name), muted);
}
bool AudioSystem::busMuted(std::string_view name) const noexcept {
    return m_impl && m_impl->buses.muted(m_impl->buses.find(name));
}
void AudioSystem::setBusSolo(std::string_view name, bool solo) {
    if (m_impl) m_impl->buses.setSolo(m_impl->buses.find(name), solo);
}
bool AudioSystem::busSolo(std::string_view name) const noexcept {
    return m_impl && m_impl->buses.solo(m_impl->buses.find(name));
}
bool AudioSystem::anySolo() const noexcept {
    return m_impl && m_impl->buses.anySolo();
}
void AudioSystem::clearAllSolo() {
    if (m_impl) m_impl->buses.clearAllSolo();
}

bool AudioSystem::isSoundReady(SoundId id) const noexcept {
    if (!m_impl) return false;
    auto* e = m_impl->soundRegistry.get(id);
    return e && e->resource && e->resource->isReady();
}

std::string_view AudioSystem::soundName(SoundId id) const noexcept {
    if (!m_impl) return {};
    auto* e = m_impl->soundRegistry.get(id);
    return e ? std::string_view(e->name) : std::string_view{};
}

void AudioSystem::updateSoundPosition(VoiceHandle h, const Spatial3D& spatial) {
    if (m_impl) m_impl->voices.updatePosition(h, spatial);
}

void AudioSystem::setSoundReadyCallback(SoundReadyCallback fn) {
    if (m_impl) m_impl->soundReadyCb = std::move(fn);
}

AudioMusicPlayer& AudioSystem::music() noexcept {
    return m_impl->musicPlayer;
}
const AudioMusicPlayer& AudioSystem::music() const noexcept {
    return m_impl->musicPlayer;
}

AudioDeviceManager& AudioSystem::devices() noexcept {
    return m_impl->device;
}
const AudioDeviceManager& AudioSystem::devices() const noexcept {
    return m_impl->device;
}
}  // namespace empire::audio