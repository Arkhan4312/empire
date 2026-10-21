#pragma once

#include <empire/engine/audio/AudioError.h>
#include <empire/engine/audio/detail/MiniAudio.h>
#include <memory>
namespace empire::audio::detail {

// RAII around ma_audio_buffer_ref. Does not own the PCM data. The caller must
// keep the source buffer alive for the entire lifetime of this ref AND any
// ma_sound built on top of it.
class MaAudioBufferRef {
public:
    MaAudioBufferRef() noexcept = default;
    ~MaAudioBufferRef();

    MaAudioBufferRef(const MaAudioBufferRef&) = delete;
    MaAudioBufferRef& operator=(const MaAudioBufferRef&) = delete;
    MaAudioBufferRef(MaAudioBufferRef&&) noexcept = default;
    MaAudioBufferRef& operator=(MaAudioBufferRef&&) noexcept;

    AudioResult<void> init(ma_format format, ma_uint32 channels,
                           const void* data, ma_uint64 frameCount);
    void uninit() noexcept;

    bool isValid() const noexcept {
        return m_ref != nullptr;
    }
    ma_audio_buffer_ref* get() noexcept {
        return m_ref.get();
    }
    const ma_audio_buffer_ref* get() const noexcept {
        return m_ref.get();
    }

    // ma_audio_buffer_ref is a ma_data source (its first member is
    // ma_data_source_base).
    ma_data_source* dataSource() noexcept {
        return m_ref ? reinterpret_cast<ma_data_source*>(m_ref.get()) : nullptr;
    }

private:
    std::unique_ptr<ma_audio_buffer_ref> m_ref;
};
}  // namespace empire::audio::detail