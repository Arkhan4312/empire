#include <empire/engine/audio/AudioChannels.h>
#include <empire/engine/audio/detail/MaAudioBufferRef.h>
#include <empire/engine/audio/detail/MaResult.h>
#include <empire/engine/log/Logger.h>

#include <utility>

namespace empire::audio::detail {
MaAudioBufferRef::~MaAudioBufferRef() {
    uninit();
}

MaAudioBufferRef::MaAudioBufferRef(MaAudioBufferRef&& other) noexcept
    : m_ref(other.m_ref), m_valid(other.m_valid) {
    other.m_ref = {};
    other.m_valid = false;
}

MaAudioBufferRef& MaAudioBufferRef::operator=(
    MaAudioBufferRef&& other) noexcept {
    if (this != &other) {
        uninit();
        m_ref = other.m_ref;
        m_valid = other.m_valid;
        other.m_ref = {};
        other.m_valid = false;
    }
    return *this;
}
AudioResult<void> MaAudioBufferRef::init(ma_format format, ma_uint32 channels,
                                         const void* data,
                                         ma_uint64 frameCount) {
    if (m_valid) {
        return AudioResult<void>::ok();
    }
    if (data == nullptr || frameCount == 0 || channels == 0 ||
        format == ma_format_unknown) {
        return AudioResult<void>::err(AudioError::InvalidArgument);
    }

    const ma_result r =
        ma_audio_buffer_ref_init(format, channels, data, frameCount, &m_ref);
    if (r != MA_SUCCESS) {
        GAME_LOG_CH_ERROR(channels::kResource,
                          "ma_audio_buffer_ref_init failed: {}",
                          maResultName(r));
        return AudioResult<void>::err(toAudioError(r));
    }
    m_valid = true;
    return AudioResult<void>::ok();
}
void MaAudioBufferRef::uninit() noexcept {
    if (!m_valid) {
        return;
    }
    ma_audio_buffer_ref_uninit(&m_ref);
    m_ref = {};
    m_valid = false;
}
}  // namespace empire::audio::detail