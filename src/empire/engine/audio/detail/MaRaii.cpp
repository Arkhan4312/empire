#include <empire/engine/audio/AudioChannels.h>
#include <empire/engine/audio/detail/MaRaii.h>
#include <empire/engine/audio/detail/MaResult.h>
#include <empire/engine/log/Logger.h>

#include <memory>
#include <utility>
namespace empire::audio::detail {

namespace {
// Map miniaudio's internal log level to our level.
log::Level mapLogLevel(ma_uint32 lvl) noexcept {
    switch (lvl) {
        case MA_LOG_LEVEL_DEBUG:
            return log::Level::Debug;
        case MA_LOG_LEVEL_INFO:
            return log::Level::Info;
        case MA_LOG_LEVEL_WARNING:
            return log::Level::Warn;
        case MA_LOG_LEVEL_ERROR:
            return log::Level::Error;
        default:
            return log::Level::Debug;
    }
}
// Forwarding callback: miniaudio -> our logger on channel audio.engine.
void maLogCallback(void* /*userdata*/, ma_uint32 level, const char* message) {
    if (message == nullptr) {
        return;
    }
    auto* logger = log::defaultLogger();
    if (logger == nullptr) {
        return;
    }
    // Bypass std::format because the message is already a C string. Potentially
    // with '%' placeholders. Pass as a literal arg via {}.
    logger->log(channels::kEngine, mapLogLevel(level),
                std::source_location::current(), "{}", message);
}
}  // namespace

// MaLog

MaLog::~MaLog() {
    uninit();
}
MaLog& MaLog::operator=(MaLog&& other) noexcept {
    if (this != &other) {
        uninit();
        m_log = std::move(other.m_log);
    }
    return *this;
}
AudioResult<void> MaLog::init() {
    if (m_log) {
        return AudioResult<void>::ok();
    }
    auto p = std::make_unique<ma_log>();
    const ma_result r = ma_log_init(nullptr, p.get());
    if (r != MA_SUCCESS) {
        GAME_LOG_CH_ERROR(channels::kEngine, "ma_log_init failed: {}",
                          maResultName(r));
        return AudioResult<void>::err(toAudioError(r));
    }
    ma_log_callback cb = ma_log_callback_init(&maLogCallback, nullptr);
    ma_log_register_callback(p.get(), cb);
    m_log = std::move(p);
    return AudioResult<void>::ok();
}

void MaLog::uninit() noexcept {
    if (!m_log) {
        return;
    }
    ma_log_uninit(m_log.get());
    m_log.reset();
}

bool MaLog::isValid() const noexcept {
    return m_log != nullptr;
}

ma_log* MaLog::get() noexcept {
    return m_log.get();
}

const ma_log* MaLog::get() const noexcept {
    return m_log.get();
}

// MaContext

MaContext::~MaContext() {
    uninit();
}

MaContext& MaContext::operator=(MaContext&& other) noexcept {
    if (this != &other) {
        uninit();
        m_ctx = std::move(other.m_ctx);
    }
    return *this;
}

AudioResult<void> MaContext::init(ma_backend* backends, ma_uint32 backendCount,
                                  const ma_context_config* cfg) {
    if (m_ctx) {
        return AudioResult<void>::ok();
    }
    auto p = std::make_unique<ma_context>();
    const ma_result r = ma_context_init(backends, backendCount, cfg, p.get());
    if (r != MA_SUCCESS) {
        GAME_LOG_CH_ERROR(channels::kDevice, "ma_context_init failed {}",
                          maResultName(r));
        return AudioResult<void>::err(toAudioError(r));
    }
    m_ctx = std::move(p);
    return AudioResult<void>::ok();
}

void MaContext::uninit() noexcept {
    if (!m_ctx) {
        return;
    }
    ma_context_uninit(m_ctx.get());
    m_ctx.reset();
}

bool MaContext::isValid() const noexcept {
    return m_ctx != nullptr;
}

ma_context* MaContext::get() noexcept {
    return m_ctx.get();
}

const ma_context* MaContext::get() const noexcept {
    return m_ctx.get();
}

// MaResourceManager

MaResourceManager::~MaResourceManager() {
    uninit();
}

MaResourceManager& MaResourceManager::operator=(
    MaResourceManager&& other) noexcept {
    if (this != &other) {
        uninit();
        m_rm = std::move(other.m_rm);
    }
    return *this;
}

AudioResult<void> MaResourceManager::init(
    const ma_resource_manager_config* cfg) {
    if (m_rm) {
        return AudioResult<void>::ok();
    }
    auto p = std::make_unique<ma_resource_manager>();
    const ma_result r = ma_resource_manager_init(cfg, p.get());
    if (r != MA_SUCCESS) {
        GAME_LOG_CH_ERROR(channels::kResource,
                          "ma_resource_manager_init failed {}",
                          maResultName(r));
        return AudioResult<void>::err(toAudioError(r));
    }
    m_rm = std::move(p);
    return AudioResult<void>::ok();
}

void MaResourceManager::uninit() noexcept {
    if (!m_rm) {
        return;
    }
    ma_resource_manager_uninit(m_rm.get());
    m_rm.reset();
}

bool MaResourceManager::isValid() const noexcept {
    return m_rm != nullptr;
}

ma_resource_manager* MaResourceManager::get() noexcept {
    return m_rm.get();
}

const ma_resource_manager* MaResourceManager::get() const noexcept {
    return m_rm.get();
}

// MaEngine

MaEngine::~MaEngine() {
    uninit();
}

MaEngine& MaEngine::operator=(MaEngine&& other) noexcept {
    if (this != &other) {
        uninit();
        m_engine = std::move(other.m_engine);
    }
    return *this;
}

AudioResult<void> MaEngine::init(const ma_engine_config* cfg) {
    if (m_engine) {
        return AudioResult<void>::ok();
    }
    auto p = std::make_unique<ma_engine>();
    const ma_result r = ma_engine_init(cfg, p.get());
    if (r != MA_SUCCESS) {
        GAME_LOG_CH_ERROR(channels::kEngine, "ma_engine_init failed {}",
                          maResultName(r));
        return AudioResult<void>::err(toAudioError(r));
    }
    m_engine = std::move(p);
    return AudioResult<void>::ok();
}

void MaEngine::uninit() noexcept {
    if (!m_engine) {
        return;
    }
    ma_engine_uninit(m_engine.get());
    m_engine.reset();
}

bool MaEngine::isValid() const noexcept {
    return m_engine != nullptr;
}

ma_engine* MaEngine::get() noexcept {
    return m_engine.get();
}

const ma_engine* MaEngine::get() const noexcept {
    return m_engine.get();
}

// MaSoundGroup

MaSoundGroup::~MaSoundGroup() {
    uninit();
}

MaSoundGroup& MaSoundGroup::operator=(MaSoundGroup&& other) noexcept {
    if (this != &other) {
        uninit();
        m_group = std::move(other.m_group);
    }
    return *this;
}

AudioResult<void> MaSoundGroup::init(ma_engine* engine, ma_uint32 flags,
                                     ma_sound_group* parent) {
    if (m_group) {
        return AudioResult<void>::ok();
    }
    if (engine == nullptr) {
        return AudioResult<void>::err(AudioError::GroupNotReady);
    }
    auto p = std::make_unique<ma_sound_group>();
    const ma_result r = ma_sound_group_init(engine, flags, parent, p.get());
    if (r != MA_SUCCESS) {
        GAME_LOG_CH_ERROR(channels::kBus, "ma_sound_group_init failed {}",
                          maResultName(r));
        return AudioResult<void>::err(toAudioError(r));
    }
    m_group = std::move(p);
    return AudioResult<void>::ok();
}

void MaSoundGroup::uninit() noexcept {
    if (!m_group) {
        return;
    }
    ma_sound_group_uninit(m_group.get());
    m_group.reset();
}

bool MaSoundGroup::isValid() const noexcept {
    return m_group != nullptr;
}

ma_sound_group* MaSoundGroup::get() noexcept {
    return m_group.get();
}

const ma_sound_group* MaSoundGroup::get() const noexcept {
    return m_group.get();
}

// MaSound

MaSound::~MaSound() {
    uninit();
}

MaSound& MaSound::operator=(MaSound&& other) noexcept {
    if (this != &other) {
        uninit();
        m_sound = std::move(other.m_sound);
    }
    return *this;
}

AudioResult<void> MaSound::initFromFile(ma_engine* engine, const char* path,
                                        ma_uint32 flags, ma_sound_group* group,
                                        ma_sound_end_proc endCallback,
                                        void* endUserData) {
    if (m_sound) {
        return AudioResult<void>::ok();
    }
    if (engine == nullptr || path == nullptr) {
        return AudioResult<void>::err(AudioError::InvalidArgument);
    }
    auto p = std::make_unique<ma_sound>();
    const ma_result r =
        ma_sound_init_from_file(engine, path, flags, group, nullptr, p.get());
    if (r != MA_SUCCESS) {
        GAME_LOG_CH_ERROR(channels::kVoice,
                          "ma_sound_init_from_file('{}') failed {}", path,
                          maResultName(r));
        return AudioResult<void>::err(toAudioError(r));
    }
    if (endCallback != nullptr) {
        ma_sound_set_end_callback(p.get(), endCallback, endUserData);
    }
    m_sound = std::move(p);
    return AudioResult<void>::ok();
}

AudioResult<void> MaSound::initFromDataSource(ma_engine* engine,
                                              ma_data_source* source,
                                              ma_uint32 flags,
                                              ma_sound_group* group) {
    if (m_sound) {
        return AudioResult<void>::ok();
    }
    if (engine == nullptr || source == nullptr) {
        return AudioResult<void>::err(AudioError::InvalidArgument);
    }
    auto p = std::make_unique<ma_sound>();
    const ma_result r =
        ma_sound_init_from_data_source(engine, source, flags, group, p.get());
    if (r != MA_SUCCESS) {
        GAME_LOG_CH_ERROR(channels::kVoice,
                          "ma_sound_init_from_data_source failed: {}",
                          maResultName(r));
        return AudioResult<void>::err(toAudioError(r));
    }
    m_sound = std::move(p);
    return AudioResult<void>::ok();
}

AudioResult<void> MaSound::initCopy(ma_engine* engine, const ma_sound* source,
                                    ma_uint32 flags, ma_sound_group* group) {
    if (m_sound) {
        return AudioResult<void>::ok();
    }
    if (engine == nullptr || source == nullptr) {
        return AudioResult<void>::err(AudioError::InvalidArgument);
    }
    auto p = std::make_unique<ma_sound>();
    const ma_result r =
        ma_sound_init_copy(engine, source, flags, group, p.get());
    if (r != MA_SUCCESS) {
        GAME_LOG_CH_ERROR(channels::kVoice, "ma_sound_init_copy failed: {}",
                          maResultName(r));
        return AudioResult<void>::err(toAudioError(r));
    }
    m_sound = std::move(p);
    return AudioResult<void>::ok();
}

void MaSound::uninit() noexcept {
    if (!m_sound) {
        return;
    }
    ma_sound_uninit(m_sound.get());
    m_sound.reset();
}

bool MaSound::isValid() const noexcept {
    return m_sound != nullptr;
}

ma_sound* MaSound::get() noexcept {
    return m_sound.get();
}

const ma_sound* MaSound::get() const noexcept {
    return m_sound.get();
}

// MaAudioBuffer

MaAudioBuffer::~MaAudioBuffer() {
    uninit();
}

MaAudioBuffer& MaAudioBuffer::operator=(MaAudioBuffer&& other) noexcept {
    if (this != &other) {
        uninit();
        m_buffer = std::move(other.m_buffer);
    }
    return *this;
}

AudioResult<void> MaAudioBuffer::init(const ma_audio_buffer_config* cfg) {
    if (m_buffer) {
        return AudioResult<void>::ok();
    }
    if (cfg == nullptr) {
        return AudioResult<void>::err(AudioError::InvalidArgument);
    }
    auto p = std::make_unique<ma_audio_buffer>();
    const ma_result r = ma_audio_buffer_init(cfg, p.get());
    if (r != MA_SUCCESS) {
        GAME_LOG_CH_ERROR(channels::kResource,
                          "ma_audio_buffer_init failed: {}", maResultName(r));
        return AudioResult<void>::err(toAudioError(r));
    }
    m_buffer = std::move(p);
    return AudioResult<void>::ok();
}

void MaAudioBuffer::uninit() noexcept {
    if (!m_buffer) {
        return;
    }
    ma_audio_buffer_uninit(m_buffer.get());
    m_buffer.reset();
}

bool MaAudioBuffer::isValid() const noexcept {
    return m_buffer != nullptr;
}

ma_audio_buffer* MaAudioBuffer::get() noexcept {
    return m_buffer.get();
}

const ma_audio_buffer* MaAudioBuffer::get() const noexcept {
    return m_buffer.get();
}

// MaDecoder

MaDecoder::~MaDecoder() {
    uninit();
}

MaDecoder& MaDecoder::operator=(MaDecoder&& other) noexcept {
    if (this != &other) {
        uninit();
        m_decoder = std::move(other.m_decoder);
    }
    return *this;
}

AudioResult<void> MaDecoder::initFromFile(const char* path,
                                          const ma_decoder_config* cfg) {
    if (m_decoder) {
        return AudioResult<void>::ok();
    }
    if (path == nullptr) {
        return AudioResult<void>::err(AudioError::InvalidArgument);
    }
    auto p = std::make_unique<ma_decoder>();
    const ma_result r = ma_decoder_init_file(path, cfg, p.get());
    if (r != MA_SUCCESS) {
        GAME_LOG_CH_ERROR(channels::kResource,
                          "ma_decoder_init_file('{}') failed: {}", path,
                          maResultName(r));
        return AudioResult<void>::err(toAudioError(r));
    }
    m_decoder = std::move(p);
    return AudioResult<void>::ok();
}

AudioResult<void> MaDecoder::initFromMemory(const void* data, std::size_t size,
                                            const ma_decoder_config* cfg) {
    if (m_decoder) {
        return AudioResult<void>::ok();
    }
    if (data == nullptr || size == 0) {
        return AudioResult<void>::err(AudioError::InvalidArgument);
    }
    auto p = std::make_unique<ma_decoder>();
    const ma_result r = ma_decoder_init_memory(data, size, cfg, p.get());
    if (r != MA_SUCCESS) {
        GAME_LOG_CH_ERROR(channels::kResource,
                          "ma_decoder_init_memory failed: {}", maResultName(r));
        return AudioResult<void>::err(toAudioError(r));
    }
    m_decoder = std::move(p);
    return AudioResult<void>::ok();
}

ma_uint64 MaDecoder::cursorInPcmFrames() const noexcept {
    if (!m_decoder) {
        return 0;
    }
    ma_uint64 f = 0;
    auto* ds = const_cast<ma_data_source*>(
        reinterpret_cast<const ma_data_source*>(m_decoder.get()));
    return ma_data_source_get_cursor_in_pcm_frames(ds, &f) == MA_SUCCESS ? f
                                                                         : 0;
}

ma_result MaDecoder::setLoopPointInPcmFrames(ma_uint64 start,
                                             ma_uint64 end) noexcept {
    if (!m_decoder) {
        return MA_INVALID_ARGS;
    }
    auto* ds = reinterpret_cast<ma_data_source*>(m_decoder.get());
    return ma_data_source_set_loop_point_in_pcm_frames(ds, start, end);
}

void MaDecoder::clearLoopPoint() noexcept {
    if (!m_decoder) {
        return;
    }
    auto* ds = reinterpret_cast<ma_data_source*>(m_decoder.get());
    ma_data_source_set_loop_point_in_pcm_frames(ds, 0, 0);
}

ma_data_source* MaDecoder::dataSource() noexcept {
    return m_decoder ? reinterpret_cast<ma_data_source*>(m_decoder.get())
                     : nullptr;
}

void MaDecoder::uninit() noexcept {
    if (!m_decoder) {
        return;
    }
    ma_decoder_uninit(m_decoder.get());
    m_decoder.reset();
}

bool MaDecoder::isValid() const noexcept {
    return m_decoder != nullptr;
}

ma_decoder* MaDecoder::get() noexcept {
    return m_decoder.get();
}

const ma_decoder* MaDecoder::get() const noexcept {
    return m_decoder.get();
}

}  // namespace empire::audio::detail