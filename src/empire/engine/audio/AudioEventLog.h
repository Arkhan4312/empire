#pragma once

#include <empire/engine/audio/AudioError.h>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace empire::audio {

enum class AudioEventKind : std::uint8_t {
    InitFailed,
    DeviceLost,
    DeviceMigration,
    VoiceStolen,
    VoiceLimitHit,
    VoicePoolExhausted,
    ResourceLoadFailed,
    BusNotFound,
    MusicTrackEnded,
};

struct AudioEvent {
    std::uint64_t tick = 0;
    AudioEventKind kind = AudioEventKind::InitFailed;
    AudioError error = AudioError::Unknown;
    std::uint32_t a = 0;
    std::uint32_t b = 0;
};

class AudioEventLog {
public:
    static constexpr std::size_t kCapacity = 512;

    void push(AudioEventKind kind, AudioError err = AudioError::Unknown,
              std::uint32_t a = 0, std::uint32_t b = 0) noexcept;

    std::vector<AudioEvent> snapshot() const;

    std::size_t size() const noexcept;
    std::uint64_t totalCount() const noexcept;

    static std::uint32_t hashStr(std::string_view s) noexcept;

private:
    std::array<AudioEvent, kCapacity> m_buf{};
    std::atomic<std::uint64_t> m_writeCursor{0};
    std::atomic<std::uint64_t> m_total{0};
};
}  // namespace empire::audio