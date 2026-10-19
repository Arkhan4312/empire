#pragma once
#include <empire/engine/audio/AudioBusRegistry.h>
#include <empire/engine/audio/AudioDeviceManager.h>
#include <empire/engine/audio/AudioEventLog.h>
#include <empire/engine/audio/AudioMusicPlayer.h>
#include <empire/engine/audio/AudioResource.h>
#include <empire/engine/audio/AudioSpatial.h>
#include <empire/engine/audio/AudioVoice.h>
#include <empire/engine/audio/AudioVoicePool.h>
#include <empire/engine/util/Handle.h>
#include <empire/engine/util/Result.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
namespace empire::audio {
struct SoundTag {};
using SoundId = util::Handle<SoundTag>;
inline constexpr SoundId kInvalidSound{};

struct SoundDesc {
    AudioResourceDesc resource;
    float baseVolume = 1.0f;
    float basePitch = 1.0f;
    float pitchVariation = 0.0f;
    bool loop = false;
    std::string bus = "sfx";
    std::uint8_t priority = 128;
    std::uint8_t maxConcurrent = 4;
};

struct AudioStats {
    // Device
    bool deviceReady = false;
    int currentDeviceIndex = -1;
    bool followsSystemDefault = true;
    bool isMigrating = false;
    float migrationGain = 1.0f;

    // Voices
    std::size_t voicesActive = 0;
    std::size_t voicesCapacity = 0;
    std::size_t voicesStolenTotal = 0;

    // Buses
    std::size_t busCount = 0;
    bool anySolo = false;

    // Resources
    std::size_t cachedResources = 0;
    std::size_t queuedAsyncJobs = 0;

    // Music
    bool musicPlaying = false;
    bool musicPaused = false;
    std::size_t musicQueueSize = 0;

    // Events (cumulative)
    std::uint64_t eventTotal = 0;
};

class AudioSystem {
public:
    AudioSystem();
    ~AudioSystem();
    AudioSystem(const AudioSystem&) = delete;
    AudioSystem& operator=(const AudioSystem&) = delete;

    // ---- Lifecycle ----

    bool init();
    void shutdown();
    bool ready() const noexcept;

    // Call once per frame from the main thread. Drives device polling,
    // migration fade state machine, voice pool cleanup, music fades, async
    // completions, and (when enabled) hot-reload scanning.
    void update(float dt);

    // Global pause: stops the miniaudio engine (SFX, voices, music — all
    // silently frozen). For music-only pause, use music().pause() instead.
    void setPaused(bool paused);
    bool paused() const noexcept;

    // ---- Sounds ----

    // Sync load. Idempotent by name. Blocks while decoding.
    AudioResult<SoundId> loadSound(std::string_view name,
                                   const SoundDesc& desc);

    // Async load. Returns immediately with a valid SoundId. The sound becomes
    // playable once the decode completes (isSoundReady(id) == true, or via
    // setSoundReadyCallback). Calling play() before that returns
    // kInvalidVoice.
    AudioResult<SoundId> loadSoundAsync(std::string_view name,
                                        const SoundDesc& desc);
    AudioResult<void> reloadSoundAsync(std::string_view name);
    // Re-scan the source file. Playing voices keep their old PCM until they
    // end; new plays use the fresh data.
    AudioResult<void> reloadSound(std::string_view name);
    AudioResult<void> reloadAllSounds();

    void unloadSound(std::string_view name);
    void unloadAllSounds();

    bool isSoundReady(SoundId id) const noexcept;
    std::string_view soundName(SoundId id) const noexcept;

    // ---- Playback ----

    VoiceHandle play(SoundId id, float volume = 1.0f, float pitch = 1.0f,
                     float panX = 0.0f);
    VoiceHandle playAt2D(SoundId id, float x, float y, float volume = 1.0f,
                         float pitch = 1.0f);
    VoiceHandle playAt3D(SoundId id, const Spatial3D& spatial,
                         float volume = 1.0f, float pitch = 1.0f);

    void stopVoice(VoiceHandle h);
    bool isVoicePlaying(VoiceHandle h) const noexcept;

    // Update position of a live 3D voice (doppler/attenuation follow).
    void updateSoundPosition(VoiceHandle h, const Spatial3D& spatial);

    // ---- Listeners ----

    void setListener2D(const Listener2D& l);
    const Listener2D& listener2D() const noexcept;

    void setListener3D(const Listener3D& l);
    const Listener3D& listener3D() const noexcept;

    // ---- Callbacks ----

    using VoiceFinishedCallback = AudioVoicePool::FinishedCallback;
    void setVoiceFinishedCallback(VoiceFinishedCallback fn);

    // Fired on the main thread from update() when an async load completes.
    // Result: ok on success, error on failure (DecodeFailed, FileNotFound,
    // ...).
    using SoundReadyCallback =
        std::function<void(SoundId id, AudioResult<void> result)>;
    void setSoundReadyCallback(SoundReadyCallback fn);

    // ---- Music ----

    void playMusic(std::string_view path, float fadeInSec = 0.8f,
                   bool loop = true);
    void stopMusic(float fadeOutSec = 0.8f);
    void crossfadeTo(std::string_view path, float durationSec = 1.2f,
                     bool loop = true);

    // Advanced music control: queue, loop-points, music-only pause, frame
    // cursor for rhythm-synced transitions.
    AudioMusicPlayer& music() noexcept;
    const AudioMusicPlayer& music() const noexcept;

    // ---- Buses ----

    // Full bus registry (find, snapshots, mute/solo iteration).
    AudioBusRegistry& buses() noexcept;
    const AudioBusRegistry& buses() const noexcept;

    // Convenience wrappers for common operations.
    void setBusMuted(std::string_view name, bool muted);
    bool busMuted(std::string_view name) const noexcept;
    void setBusSolo(std::string_view name, bool solo);
    bool busSolo(std::string_view name) const noexcept;
    bool anySolo() const noexcept;
    void clearAllSolo();

    // ---- Volume ----

    void setMasterVolume(float v);
    void setSfxVolume(float v);
    void setMusicVolume(float v);
    float masterVolume() const noexcept;
    float sfxVolume() const noexcept;
    float musicVolume() const noexcept;

    void setBusVolume(std::string_view busName, float v);
    float busVolume(std::string_view busName) const noexcept;

    // Fade every bus volume to a target snapshot over fadeSec seconds.
    void applyBusSnapshot(const AudioBusSnapshot& snap, float fadeSec = 0.5f);

    // ---- Resource cache ----

    void evictResource(std::string_view logicalPath);
    void clearResourceCache();
    std::size_t cachedResourceCount() const noexcept;

    // Dev-only: poll mtimes once per second and auto-reload changed sounds.
    void setAutoHotReloadEnabled(bool enabled) noexcept;
    bool autoHotReloadEnabled() const noexcept;

    // ---- Diagnostics ----

    AudioStats stats() const;
    std::string diagnosticsJson() const;
    bool dumpDiagnostics(std::string_view path) const;

    std::size_t activeVoiceCount() const noexcept;
    std::size_t stolenVoiceCount() const noexcept;

    // ---- Devices ----

    const std::vector<AudioDeviceInfo>& listOutputDevices() const noexcept;
    int currentOutputDevice() const noexcept;

    // Async: returns true if the request was accepted. Completion arrives via
    // AudioDeviceManager::MigrationCallback. Poll isMigrating() or observe
    // migrationGain() to track the fade.
    bool setOutputDevice(int index);

    // Migration tuning (advanced).
    AudioDeviceManager& devices() noexcept;
    const AudioDeviceManager& devices() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
}  // namespace empire::audio