#pragma once

#include <empire/engine/audio/AudioError.h>
#include <empire/engine/audio/AudioEventLog.h>
#include <empire/engine/audio/detail/MiniAudio.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace empire::audio {

class AudioDeviceManager;
class AudioBusRegistry;

class AudioMusicPlayer {
public:
    static constexpr float kDefaultFadeIn = 0.8f;
    static constexpr float kDefaultFadeOut = 0.8f;
    static constexpr float kDefaultCrossfade = 1.2f;
    static constexpr float kPauseFade = 0.2f;

    AudioMusicPlayer();
    ~AudioMusicPlayer();
    AudioMusicPlayer(const AudioMusicPlayer&) = delete;
    AudioMusicPlayer& operator=(const AudioMusicPlayer&) = delete;

    // Bind to current device manager + bus registry. Idempotent.
    AudioResult<void> init(AudioDeviceManager& device, AudioBusRegistry& buses);

    // tear down all slots. Called on shutdown.
    void uninit() noexcept;

    // Playback

    // Play 'path'. If something else is already playing, this performs an
    // equal-power crossfade of length fadeInSec (both tracks overlap). If
    // 'path' is already the current track - no-op
    // NOTE: the player owns exactly 2 slots. Calling play()/crossfateTo()
    // while two other tracks are active will forcibly release the oldest slot
    // without a fade-out (state is destroyed synchronously). This is acceptable
    // for a music track but should not be used for layering
    void play(std::string_view path, float fadeInSec = kDefaultFadeIn,
              bool loop = true);

    // Enqueue for after the current (non-looping) track finishes.
    void playNext(std::string_view path, float fadeInSec = kDefaultFadeIn,
                  bool loop = true);

    // Explicit crossfade (same as play() with durationSec). Exists for
    // readability in rhytm-driven scenarios.
    void crossfadeTo(std::string_view path,
                     float durationSec = kDefaultCrossfade, bool loop = true);

    // Fade out all active slots and drop the queue.
    void stop(float fadeOutSec = kDefaultFadeOut);

    void clearQueue() noexcept;
    std::size_t queuedCount() const noexcept;

    // Music-only pause (SFX unaffected)
    // Fades out then ma_sound_stop; cursor is preserved by the decoder.
    void pause(float fadeOutSec = kPauseFade);
    void resume(float fadeInSec = kPauseFade);
    bool isPaused() const noexcept;

    // Loop-points for "subsequent" plays
    // Whole-track looping when start == end (both zero)
    void setLoopPoints(ma_uint64 startFrame, ma_uint64 endFrame);
    void clearLoopPoints();

    // Diagnostics
    bool isPlaying() const noexcept;
    const std::string& currentPath() const noexcept;
    ma_uint64 currentFrame() const noexcept;

    // Main-thread tick
    void update(float dt);

    // Migration hooks - called by AudioSystem
    void onBeforeMigration();
    void onAfterMigration();
    void setEventLog(AudioEventLog* log) noexcept {
        m_events = log;
    }

private:
    AudioEventLog* m_events = nullptr;
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
}  // namespace empire::audio