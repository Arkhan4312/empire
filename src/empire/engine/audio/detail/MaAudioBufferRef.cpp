#include <empire/engine/audio/AudioChannels.h>
#include <empire/engine/audio/detail/MaAudioBufferRef.h>
#include <empire/engine/audio/detail/MaResult.h>
#include <empire/engine/log/Logger.h>

#include <utility>
#include <memory>
namespace empire::audio::detail {
MaAudioBufferRef::~MaAudioBufferRef() {
    uninit();
}

MaAudioBufferRef& MaAudioBufferRef::operator=(MaAudioBufferRef&& other) noexcept {
    if (this != &other) {
        uninit();
        m_ref = std::move(other.m_ref);
    }
    return *this;
}

AudioResult<void> MaAudioBufferRef::init(ma_format format, ma_uint32 channels,
                                         const void* data,
                                         ma_uint64 frameCount) {
    if (m_ref) {
        return AudioResult<void>::ok();
    }
    if (data == nullptr || frameCount == 0 || channels == 0 ||
        format == ma_format_unknown) {
        return AudioResult<void>::err(AudioError::InvalidArgument);
    }
    auto p = std::make_unique<ma_audio_buffer_ref>();
    const ma_result r =
        ma_audio_buffer_ref_init(format, channels, data, frameCount, p.get());
    if (r != MA_SUCCESS) {
        GAME_LOG_CH_ERROR(channels::kResource,
                          "ma_audio_buffer_ref_init failed: {}",
                          maResultName(r));
        return AudioResult<void>::err(toAudioError(r));
    }
    m_ref = std::move(p);
    return AudioResult<void>::ok();
}
void MaAudioBufferRef::uninit() noexcept {
    if (!m_ref) {
        return;
    }
    ma_audio_buffer_ref_uninit(m_ref.get());
    m_ref.reset();
}
}  // namespace empire::audio::detail