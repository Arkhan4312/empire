#pragma once

#include <empire/engine/audio/AudioError.h>
#include <empire/engine/audio/detail/MiniAudio.h>

#include <string_view>

namespace empire::audio::detail {
const char* maResultName(ma_result r) noexcept;

AudioError toAudioError(ma_result r) noexcept;

AudioResult<void> maCheck(ma_result r) noexcept;
}  // namespace empire::audio::detail