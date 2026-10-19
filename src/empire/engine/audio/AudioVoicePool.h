#pragma once

#include <empire/engine/audio/AudioError.h>
#include <empire/engine/audio/AudioEventLog.h>
#include <empire/engine/audio/AudioResource.h>
#include <empire/engine/audio/AudioSpatial.h>
#include <empire/engine/audio/AudioVoice.h>
#include <empire/engine/audio/detail/MaAudioBufferRef.h>
#include <empire/engine/audio/detail/MaRaii.h>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace empire::audio {

// Everything needed to start one voice. Populated by AudioSystem; the pool
// itself has no knowledge of SoundEntry / bus names / registry.
//
// Why a vaue struct and not a pointer to SoundEntry: the pool is meant to be
// unit-testable in isolation, with no dependency on AudioSystem internals.
struct VoiceStartDesc {
    std::shared_ptr<const AudioResource>
        resource;                           // keeps PCM alive while playing
    detail::MaSoundGroup* group = nullptr;  // resolved bus group; mandatory

    float volume = 1.0f;
    float pitch = 1.0f;
    float pan = 0.0f;
    bool loop = false;

    std::uint8_t priority = 128;  // higher - more important
    std::uint8_t maxConcurrent =
        4;  // cap on simultaneous instances of THIS sound
    std::uint32_t soundGroupKey = 0;  // opaque, used for per-sound counting

    std::optional<Spatial2D> spatial2D;
    std::optional<Spatial3D> spatial3D;
};

struct VoiceStartOutcome {
    VoiceHandle handle = kInvalidVoice;
    VoiceStartResult result = VoiceStartResult::NotInitialized;

    bool ok() const noexcept {
        return result == VoiceStartResult::Started;
    }
};

class AudioVoicePool {
public:
    static constexpr std::size_t kMaxVoices = 64;

    AudioVoicePool();
    ~AudioVoicePool();
    AudioVoicePool(const AudioVoicePool&) = delete;
    AudioVoicePool& operator=(const AudioVoicePool&) = delete;

    // Bind to an engine. Re-binding after a migration: uninit() then init().
    // Slotl are always zeroed between the two; any live voice is killed.
    AudioResult<void> init(ma_engine* engine);
    void uninit() noexcept;

    // Attempt to start a voice. On success, handle identifies it for stop() and
    // isPlaying(). On failure, result explains why.
    VoiceStartOutcome start(const VoiceStartDesc& desc);

    // Immediate stop. Slot is released synchonously (before return), user
    // callback (if any) is queued and fired at the end of the call. No-op if
    // handle is stale or invalid.
    void stop(VoiceHandle h);

    // Stop every voice whose soundGroupKey matchs. Used by unloadSound()
    void stopByGroupKey(std::uint32_t key);

    // True if handle refers to a currently playing slot
    bool isPlaying(VoiceHandle h) const noexcept;

    // Main-thread. Reclaims slots wwhose sounds ended naturally.
    void update(float dt);

    // diagnostics.
    std::size_t activeCount() const noexcept {
        return m_activeCount;
    }
    std::size_t stolenTotal() const noexcept {
        return m_stolenTotal;
    }
    static constexpr std::size_t capacity() noexcept {
        return kMaxVoices;
    }

    void setListener2D(const Listener2D& l) noexcept {
        m_listener2D = l;
    }
    const Listener2D& listener2D() const noexcept {
        return m_listener2D;
    }
    // Callback for voice endings. Invoked from the main thread.
    using FinishedCallback = std::function<void(const VoiceFinishEvent&)>;
    void setFinishedCallback(FinishedCallback fn);

    float computePan2D(const Spatial2D& src) const noexcept;
    void setEventLog(AudioEventLog* log) noexcept {
        m_events = log;
    }
    void updatePosition(VoiceHandle h, const Spatial3D& spatial);

private:
    AudioEventLog* m_events = nullptr;

    enum class SlotState : std::uint8_t {
        Free,
        Playing,
    };

    struct Slot {
        SlotState state = SlotState::Free;
        std::uint32_t generation = 1;
        std::uint32_t soundGroupKey = 0;
        std::uint8_t priority = 0;
        std::uint64_t startTick = 0;

        // Keeps decoded PCM alive across hot-reload / unload sound
        std::shared_ptr<const AudioResource> resource;

        // 'ref' MUST be declared befor 'sound'
        detail::MaAudioBufferRef ref;
        detail::MaSound sound;

        std::optional<Spatial2D> spatial2D;
        // set by the audio-thread end callaback; cleared by the main thread on
        // release. Atomic because it crosses threads.
        std::atomic<bool> finished{false};
    };
    // Audio-thread entry point.
    static void endCallbackTrampoline(void* userdata, ma_sound*);

    Slot* slotFromHandle(VoiceHandle h) noexcept;
    const Slot* slotFromHandle(VoiceHandle h) const noexcept;

    Slot* findFreeSlot() noexcept;
    Slot* findOldestSameSound(std::uint32_t groupKey) noexcept;
    // Lowest priority; ties broken by oldest startTick.
    Slot* findGlobalStealCandidate() noexcept;

    // Stop everything in 's', bump generation, push event if requested.
    void releaseSlot(Slot& s, VoiceStopReason reason, bool emitEvent);

    // Init the given (already free) slot with the new sound.
    AudioResult<void> armSlot(Slot& s, const VoiceStartDesc& desc);

    void firePending();

    std::array<Slot, kMaxVoices> m_slots{};
    ma_engine* m_engine = nullptr;

    std::size_t m_activeCount = 0;
    std::size_t m_stolenTotal = 0;
    std::uint64_t m_tickCounter = 0;
    Listener2D m_listener2D{};

    FinishedCallback m_finished;
    std::vector<VoiceFinishEvent> m_pendingEvents;
};
}  // namespace empire::audio