#pragma once

#include <empire/engine/audio/AudioError.h>
#include <empire/engine/audio/detail/MiniAudio.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace empire::audio {
enum class LoadMode : std::uint8_t {
    Auto,    // decide by extension
    Decode,  // fully decode into RAM (default for short SFX)
    Stream,  // stream from disk per playback (music / long ambience)
};

struct AudioResourceDesc {
    std::string path;  // logical path, resolved via asset::resolve
    LoadMode mode = LoadMode::Auto;
};

enum class ResourceState : std::uint8_t {
    Unloaded,
    Loading,
    Ready,
    Failed,
};

// Immutable after load(). Thread-unsafe (main-thread only).
// Does not depend on ma_engine or ma_context - survives device migration
class AudioResource {
public:
    AudioResource() = default;
    ~AudioResource();

    AudioResource(const AudioResource&) = delete;
    AudioResource& operator=(const AudioResource&) = delete;
    AudioResource(AudioResource&&) noexcept;
    AudioResource& operator=(AudioResource&&) noexcept;

    // Synchronous load. Never leaves the object in a partial state.
    AudioResult<void> load(const AudioResourceDesc& desc);

    void markLoading() noexcept;
    AudioResult<void> loadInBackground(const AudioResourceDesc& desc);

    void uninit() noexcept;

    bool isReady() const noexcept {
        return m_state.load(std::memory_order_acquire) == ResourceState::Ready;
    }
    bool isLoading() const noexcept {
        return m_state.load(std::memory_order_acquire) ==
               ResourceState::Loading;
    }
    ResourceState state() const noexcept {
        return m_state.load(std::memory_order_acquire);
    }
    bool isStreamed() const noexcept {
        return m_streamed;
    }
    bool isDecoded() const noexcept {
        return isReady() && !m_streamed;
    }
    const std::string& logicalPath() const noexcept {
        return m_logicalPath;
    }
    const std::string& resolvedPath() const noexcept {
        return m_resolvedPath;
    }
    const std::filesystem::file_time_type& mtime() const noexcept {
        return m_mtime;
    }

    ma_format format() const noexcept {
        return m_format;
    }
    ma_uint32 channels() const noexcept {
        return m_channels;
    }
    ma_uint32 sampleRate() const noexcept {
        return m_sampleRate;
    }
    ma_uint64 frameCount() const noexcept {
        return m_frameCount;
    }

    // Decoded-only: interLeaved f32 PCM. Lifetime == lifetime of this
    // AudioResource.
    const void* pcmData() const noexcept {
        return m_pcm.empty() ? nullptr : m_pcm.data();
    }

    // Streamde-only: absolute path, safe to pass to ma_sound_init_from_file
    const char* streamPath() const noexcept {
        return m_resolvedPath.empty() ? nullptr : m_resolvedPath.c_str();
    }

private:
    AudioResult<void> loadImpl(const AudioResourceDesc& desc, bool background);
    std::atomic<ResourceState> m_state{ResourceState::Unloaded};
    bool m_streamed = false;

    std::string m_logicalPath;
    std::string m_resolvedPath;
    std::filesystem::file_time_type m_mtime{};

    ma_format m_format = ma_format_unknown;
    ma_uint32 m_channels = 0;
    ma_uint32 m_sampleRate = 0;
    ma_uint64 m_frameCount = 0;

    // Decoded PCM (f32 interleaved). Empty for streamed resources.
    std::vector<std::byte> m_pcm;
};
}  // namespace empire::audio