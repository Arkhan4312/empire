#pragma once

#include <empire/engine/audio/detail/MaRaii.h>
#include <empire/engine/util/Handle.h>

#include <cstdint>
#include <string>
#include <vector>

namespace empire::audio {

struct BusTag {};
using BusHandle = util::Handle<BusTag>;
inline constexpr BusHandle kInvalidBus{};

// Runtime state of the mixer bus. Corresponds 1:1 with a ma_sound_group.
// Lifetime Model:
// - AudioBus surveves device migration
// - The embedded MaSoundGroup does not survive migraion.
struct AudioBus {
    std::string name;
    BusHandle parent{};               // invalid for root
    std::vector<BusHandle> children;  // bookkeping; used for solo computetion
    bool isUserSet = false;
    // Target volume (0..1). What setVolume() / applySnapshot() write.
    float volume = 1.0f;

    // Applied volume. Differs from 'volume' only during an animated snapshot
    // fade; otherwise they're identical
    float currentVolume = 1.0f;

    // Per-bus fade state. Only touched by applySnapshot().
    float fadeStartVolume = 1.0f;
    float fadeSeconds = 0.0f;
    float fadeElapsed = 0.0f;

    // User facing flags.
    bool muted = false;
    bool solo = false;

    // Derived. Recomputed whenever any bus's solo changes. A bus is "soloKill"
    // when at least one bus is solo'd and this bus is neither solo'd nor an
    // ancestor of any solo'd bus/
    bool soloKill = false;

    // engine-bound. Invalid between AudioBusRegistry::uninit() and init().
    detail::MaSoundGroup group;
};
}  // namespace empire::audio