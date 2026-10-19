#pragma once

#include <empire/engine/audio/AudioError.h>
#include <empire/engine/audio/detail/MiniAudio.h>

namespace empire::audio::detail {

// ma_log
class MaLog {
public:
    MaLog() noexcept = default;
    ~MaLog();
    MaLog(const MaLog&) = delete;
    MaLog& operator=(const MaLog&) = delete;
    MaLog(MaLog&&) noexcept;
    MaLog& operator=(MaLog&&) noexcept;

    AudioResult<void> init();
    void uninit() noexcept;

    bool isValid() const noexcept;
    ma_log* get() noexcept;
    const ma_log* get() const noexcept;

private:
    ma_log m_log{};
    bool m_valid = false;
};

// ma_context
class MaContext {
public:
    MaContext() noexcept = default;
    ~MaContext();
    MaContext(const MaContext&) = delete;
    MaContext& operator=(const MaContext&) = delete;
    MaContext(MaContext&&) noexcept;
    MaContext& operator=(MaContext&&) noexcept;

    AudioResult<void> init(ma_backend* backends, ma_uint32 backendCount,
                           const ma_context_config* cfg);
    void uninit() noexcept;

    bool isValid() const noexcept;
    ma_context* get() noexcept;
    const ma_context* get() const noexcept;

private:
    ma_context m_ctx{};
    bool m_valid = false;
};

// ma_resource_manager
class MaResourceManager {
public:
    MaResourceManager() noexcept = default;
    ~MaResourceManager();
    MaResourceManager(const MaResourceManager&) = delete;
    MaResourceManager& operator=(const MaResourceManager&) = delete;
    MaResourceManager(MaResourceManager&&) noexcept;
    MaResourceManager& operator=(MaResourceManager&&) noexcept;
    AudioResult<void> init(const ma_resource_manager_config* cfg);
    void uninit() noexcept;

    bool isValid() const noexcept;
    ma_resource_manager* get() noexcept;
    const ma_resource_manager* get() const noexcept;

private:
    ma_resource_manager m_rm{};
    bool m_valid = false;
};

// ma_engine

class MaEngine {
public:
    MaEngine() noexcept = default;
    ~MaEngine();
    MaEngine(const MaEngine&) = delete;
    MaEngine& operator=(const MaEngine&) = delete;
    MaEngine(MaEngine&&) noexcept;
    MaEngine& operator=(MaEngine&&) noexcept;

    AudioResult<void> init(const ma_engine_config* cfg);
    void uninit() noexcept;

    bool isValid() const noexcept;
    ma_engine* get() noexcept;
    const ma_engine* get() const noexcept;

private:
    ma_engine m_engine{};
    bool m_valid = false;
};

// ma_sound_group
class MaSoundGroup {
public:
    MaSoundGroup() noexcept = default;
    ~MaSoundGroup();
    MaSoundGroup(const MaSoundGroup&) = delete;
    MaSoundGroup& operator=(const MaSoundGroup&) = delete;
    MaSoundGroup(MaSoundGroup&&) noexcept;
    MaSoundGroup& operator=(MaSoundGroup&&) noexcept;

    AudioResult<void> init(ma_engine* engine, ma_uint32 flags,
                           ma_sound_group* parent);
    void uninit() noexcept;

    bool isValid() const noexcept;
    ma_sound_group* get() noexcept;
    const ma_sound_group* get() const noexcept;

private:
    ma_sound_group m_group{};
    bool m_valid = false;
};

// ma_sound
class MaSound {
public:
    MaSound() noexcept = default;
    ~MaSound();
    MaSound(const MaSound&) = delete;
    MaSound& operator=(const MaSound&) = delete;
    MaSound(MaSound&&) noexcept;
    MaSound& operator=(MaSound&&) noexcept;

    AudioResult<void> initFromFile(ma_engine* engine, const char* path,
                                   ma_uint32 flags, ma_sound_group* group,
                                   ma_sound_end_proc endCallback = nullptr,
                                   void* endUserData = nullptr);
    AudioResult<void> initFromDataSource(ma_engine* engine,
                                         ma_data_source* source,
                                         ma_uint32 flags,
                                         ma_sound_group* group);
    AudioResult<void> initCopy(ma_engine* engine, const ma_sound* source,
                               ma_uint32 flags, ma_sound_group* group);

    void uninit() noexcept;

    bool isValid() const noexcept;
    ma_sound* get() noexcept;
    const ma_sound* get() const noexcept;

private:
    ma_sound m_sound{};
    bool m_valid = false;
};

// ma_audio_buffer
class MaAudioBuffer {
public:
    MaAudioBuffer() noexcept = default;
    ~MaAudioBuffer();
    MaAudioBuffer(const MaAudioBuffer&) = delete;
    MaAudioBuffer& operator=(const MaAudioBuffer&) = delete;
    MaAudioBuffer(MaAudioBuffer&&) noexcept;
    MaAudioBuffer& operator=(MaAudioBuffer&&) noexcept;

    AudioResult<void> init(const ma_audio_buffer_config* cfg);
    void uninit() noexcept;

    bool isValid() const noexcept;
    ma_audio_buffer* get() noexcept;
    const ma_audio_buffer* get() const noexcept;

private:
    ma_audio_buffer m_buffer{};
    bool m_valid = false;
};

// ma_decoder
class MaDecoder {
public:
    MaDecoder() noexcept = default;
    ~MaDecoder();
    MaDecoder(const MaDecoder&) = delete;
    MaDecoder& operator=(const MaDecoder&) = delete;
    MaDecoder(MaDecoder&&) noexcept;
    MaDecoder& operator=(MaDecoder&&) noexcept;

    AudioResult<void> initFromFile(const char* path,
                                   const ma_decoder_config* cfg);
    AudioResult<void> initFromMemory(const void* data, std::size_t size,
                                     const ma_decoder_config* cfg);
    ma_uint64 cursorInPcmFrames() const noexcept;
    ma_result setLoopPointInPcmFrames(ma_uint64 start, ma_uint64 end) noexcept;
    void clearLoopPoint() noexcept;
    void uninit() noexcept;
    ma_data_source* dataSource() noexcept;
    bool isValid() const noexcept;
    ma_decoder* get() noexcept;
    const ma_decoder* get() const noexcept;

private:
    ma_decoder m_decoder{};
    bool m_valid = false;
};

}  // namespace empire::audio::detail