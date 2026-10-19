#pragma once

#include <empire/engine/util/Handle.h>

#include <cstdint>

namespace empire::audio {

struct VoiceTag {};
using VoiceHandle = util::Handle<VoiceTag>;
inline constexpr VoiceHandle kInvalidVoice{};

enum class VoiceStartResult : std::uint8_t {
    Started,           // voice acquired and playing
    NoFreeVoice,       // pool full, nothing suitable to steal
    TooLowPriority,    // pool full, all candidates had strictly higher priority
    PerSoundLimitHit,  // sound's maxConcurrent already at cap, could not reuse
    InvalidResource,   // resource not ready / missing
    NotInitialized,    // pool has no engine
};

// Why a voice stopped. Delivered via AudioVoicePool::setFinishedCallback.
enum class VoiceStopReason : std::uint8_t {
    Finished,  // sound reached natural end
    Stolen,    // replaced by a higher-priority voice
    Stopped,   // user called stopVoice()
};

struct VoiceFinishEvent {
    VoiceHandle handle{};
    std::uint32_t soundGroupKey =
        0;  // opaque id of the SoundId this voice was playing
    VoiceStopReason reason = VoiceStopReason::Finished;
};
}  // namespace empire::audio