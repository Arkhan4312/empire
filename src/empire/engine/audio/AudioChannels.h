#pragma once

#include <empire/engine/log/Logger.h>

namespace empire::audio::channels {

inline constexpr log::Channel kDevice = "audio.device";
inline constexpr log::Channel kEngine = "audio.engine";
inline constexpr log::Channel kResource = "audio.resource";
inline constexpr log::Channel kBus = "audio.bus";
inline constexpr log::Channel kVoice = "audio.voice";
inline constexpr log::Channel kMusic = "audio.music";
inline constexpr log::Channel kSpatial = "audio.spatial";
inline constexpr log::Channel kEvent = "audio.event";

void registerDefaults(log::Logger& logger);
}  // namespace empire::audio::channels