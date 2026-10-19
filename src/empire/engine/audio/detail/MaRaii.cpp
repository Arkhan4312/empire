#include <empire/engine/audio/AudioChannels.h>
#include <empire/engine/audio/detail/MaRaii.h>
#include <empire/engine/audio/detail/MaResult.h>
#include <empire/engine/log/Logger.h>

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

MaLog::MaLog(MaLog&& other) noexcept
    : m_log(other.m_log), m_valid(other.m_valid) {
    other.m_log = {};
    other.m_valid = false;
}

MaLog& MaLog::operator=(MaLog&& other) noexcept {
    if (this != &other) {
        uninit();
        m_log = other.m_log;
        m_valid = other.m_valid;
        other.m_log = {};
        other.m_valid = false;
    }
    return *this;
}

AudioResult<void> MaLog::init() {
    if (m_valid) {
        return AudioResult<void>::ok();
    }
    const ma_result r = ma_log_init(nullptr, &m_log);
    if (r != MA_SUCCESS) {
        GAME_LOG_CH_ERROR(channels::kEngine, "ma_log_init failed: {}",
                          maResultName(r));
        return AudioResult<void>::err(toAudioError(r));
    }
    ma_log_callback cb = ma_log_callback_init(&maLogCallback, nullptr);
    ma_log_register_callback(&m_log, cb);

    m_valid = true;
    return AudioResult<void>::ok();
}

void MaLog::uninit() noexcept {
    if (!m_valid) {
        return;
    }
    ma_log_uninit(&m_log);
    m_log = {};
    m_valid = false;
}

bool MaLog::isValid() const noexcept {
    return m_valid;
}

ma_log* MaLog::get() noexcept {
    return m_valid ? &m_log : nullptr;
}

const ma_log* MaLog::get() const noexcept {
    return m_valid ? &m_log : nullptr;
}

// MaContext

MaContext::~MaContext() {
    uninit();
}

MaContext::MaContext(MaContext&& other) noexcept
    : m_ctx(other.m_ctx), m_valid(other.m_valid) {
    other.m_ctx = {};
    other.m_valid = false;
}

MaContext& MaContext::operator=(MaContext&& other) noexcept {
    if (this != &other) {
        uninit();
        m_ctx = other.m_ctx;
        m_valid = other.m_valid;
        other.m_ctx = {};
        other.m_valid = false;
    }
    return *this;
}

AudioResult<void> MaContext::init(ma_backend* backends, ma_uint32 backendCount,
                                  const ma_context_config* cfg) {
    if (m_valid) {
        return AudioResult<void>::ok();
    }

    const ma_result r = ma_context_init(backends, backendCount, cfg, &m_ctx);
    if (r != MA_SUCCESS) {
        GAME_LOG_CH_ERROR(channels::kDevice, "ma_context_init failed {}",
                          maResultName(r));
        return AudioResult<void>::err(toAudioError(r));
    }
    m_valid = true;
    return AudioResult<void>::ok();
}

void MaContext::uninit() noexcept {
    if (!m_valid) {
        return;
    }
    ma_context_uninit(&m_ctx);
    m_ctx = {};
    m_valid = false;
}

bool MaContext::isValid() const noexcept {
    return m_valid;
}

ma_context* MaContext::get() noexcept {
    return m_valid ? &m_ctx : nullptr;
}

const ma_context* MaContext::get() const noexcept {
    return m_valid ? &m_ctx : nullptr;
}

// MaResourceManager

MaResourceManager::~MaResourceManager() {
    uninit();
}

MaResourceManager::MaResourceManager(MaResourceManager&& other) noexcept
    : m_rm(other.m_rm), m_valid(other.m_valid) {
    other.m_rm = {};
    other.m_valid = false;
}

MaResourceManager& MaResourceManager::operator=(
    MaResourceManager&& other) noexcept {
    if (this != &other) {
        uninit();
        m_rm = other.m_rm;
        m_valid = other.m_valid;
        other.m_rm = {};
        other.m_valid = false;
    }
    return *this;
}

AudioResult<void> MaResourceManager::init(
    const ma_resource_manager_config* cfg) {
    if (m_valid) {
        return AudioResult<void>::ok();
    }

    const ma_result r = ma_resource_manager_init(cfg, &m_rm);
    if (r != MA_SUCCESS) {
        GAME_LOG_CH_ERROR(channels::kResource,
                          "ma_resource_manager_init failed {}",
                          maResultName(r));
        return AudioResult<void>::err(toAudioError(r));
    }
    m_valid = true;
    return AudioResult<void>::ok();
}

void MaResourceManager::uninit() noexcept {
    if (!m_valid) {
        return;
    }
    ma_resource_manager_uninit(&m_rm);
    m_rm = {};
    m_valid = false;
}

bool MaResourceManager::isValid() const noexcept {
    return m_valid;
}

ma_resource_manager* MaResourceManager::get() noexcept {
    return m_valid ? &m_rm : nullptr;
}

const ma_resource_manager* MaResourceManager::get() const noexcept {
    return m_valid ? &m_rm : nullptr;
}

// MaEngine

MaEngine::~MaEngine() {
    uninit();
}

MaEngine::MaEngine(MaEngine&& other) noexcept
    : m_engine(other.m_engine), m_valid(other.m_valid) {
    other.m_engine = {};
    other.m_valid = false;
}

MaEngine& MaEngine::operator=(MaEngine&& other) noexcept {
    if (this != &other) {
        uninit();
        m_engine = other.m_engine;
        m_valid = other.m_valid;
        other.m_engine = {};
        other.m_valid = false;
    }
    return *this;
}

AudioResult<void> MaEngine::init(const ma_engine_config* cfg) {
    if (m_valid) {
        return AudioResult<void>::ok();
    }

    const ma_result r = ma_engine_init(cfg, &m_engine);
    if (r != MA_SUCCESS) {
        GAME_LOG_CH_ERROR(channels::kEngine, "ma_engine_init failed {}",
                          maResultName(r));
        return AudioResult<void>::err(toAudioError(r));
    }
    m_valid = true;
    return AudioResult<void>::ok();
}

void MaEngine::uninit() noexcept {
    if (!m_valid) {
        return;
    }
    ma_engine_uninit(&m_engine);
    m_engine = {};
    m_valid = false;
}

bool MaEngine::isValid() const noexcept {
    return m_valid;
}

ma_engine* MaEngine::get() noexcept {
    return m_valid ? &m_engine : nullptr;
}

const ma_engine* MaEngine::get() const noexcept {
    return m_valid ? &m_engine : nullptr;
}

// MaSoundGroup

MaSoundGroup::~MaSoundGroup() {
    uninit();
}

MaSoundGroup::MaSoundGroup(MaSoundGroup&& other) noexcept
    : m_group(other.m_group), m_valid(other.m_valid) {
    other.m_group = {};
    other.m_valid = false;
}

MaSoundGroup& MaSoundGroup::operator=(MaSoundGroup&& other) noexcept {
    if (this != &other) {
        uninit();
        m_group = other.m_group;
        m_valid = other.m_valid;
        other.m_group = {};
        other.m_valid = false;
    }
    return *this;
}

AudioResult<void> MaSoundGroup::init(ma_engine* engine, ma_uint32 flags,
                                     ma_sound_group* parent) {
    if (m_valid) {
        return AudioResult<void>::ok();
    }
    if (engine == nullptr) {
        return AudioResult<void>::err(AudioError::GroupNotReady);
    }

    const ma_result r = ma_sound_group_init(engine, flags, parent, &m_group);
    if (r != MA_SUCCESS) {
        GAME_LOG_CH_ERROR(channels::kBus, "ma_sound_group_init failed {}",
                          maResultName(r));
        return AudioResult<void>::err(toAudioError(r));
    }
    m_valid = true;
    return AudioResult<void>::ok();
}

void MaSoundGroup::uninit() noexcept {
    if (!m_valid) {
        return;
    }
    ma_sound_group_uninit(&m_group);
    m_group = {};
    m_valid = false;
}

bool MaSoundGroup::isValid() const noexcept {
    return m_valid;
}

ma_sound_group* MaSoundGroup::get() noexcept {
    return m_valid ? &m_group : nullptr;
}

const ma_sound_group* MaSoundGroup::get() const noexcept {
    return m_valid ? &m_group : nullptr;
}

// MaSound

MaSound::~MaSound() {
    uninit();
}

MaSound::MaSound(MaSound&& other) noexcept
    : m_sound(other.m_sound), m_valid(other.m_valid) {
    other.m_sound = {};
    other.m_valid = false;
}

MaSound& MaSound::operator=(MaSound&& other) noexcept {
    if (this != &other) {
        uninit();
        m_sound = other.m_sound;
        m_valid = other.m_valid;
        other.m_sound = {};
        other.m_valid = false;
    }
    return *this;
}

AudioResult<void> MaSound::initFromFile(ma_engine* engine, const char* path,
                                        ma_uint32 flags, ma_sound_group* group,
                                        ma_sound_end_proc endCallback,
                                        void* endUserData) {
    if (m_valid) {
        return AudioResult<void>::ok();
    }
    if (engine == nullptr || path == nullptr) {
        return AudioResult<void>::err(AudioError::InvalidArgument);
    }

    const ma_result r =
        ma_sound_init_from_file(engine, path, flags, group, nullptr, &m_sound);
    if (r != MA_SUCCESS) {
        GAME_LOG_CH_ERROR(channels::kVoice,
                          "ma_sound_init_from_file('{}') failed {}", path,
                          maResultName(r));
        return AudioResult<void>::err(toAudioError(r));
    }
    if (endCallback != nullptr) {
        ma_sound_set_end_callback(&m_sound, endCallback, endUserData);
    }
    m_valid = true;
    return AudioResult<void>::ok();
}

AudioResult<void> MaSound::initFromDataSource(ma_engine* engine,
                                              ma_data_source* source,
                                              ma_uint32 flags,
                                              ma_sound_group* group) {
    if (m_valid) {
        return AudioResult<void>::ok();
    }
    if (engine == nullptr || source == nullptr) {
        return AudioResult<void>::err(AudioError::InvalidArgument);
    }
    const ma_result r =
        ma_sound_init_from_data_source(engine, source, flags, group, &m_sound);
    if (r != MA_SUCCESS) {
        GAME_LOG_CH_ERROR(channels::kVoice,
                          "ma_sound_init_from_data_source failed: {}",
                          maResultName(r));
        return AudioResult<void>::err(toAudioError(r));
    }
    m_valid = true;
    return AudioResult<void>::ok();
}

AudioResult<void> MaSound::initCopy(ma_engine* engine, const ma_sound* source,
                                    ma_uint32 flags, ma_sound_group* group) {
    if (m_valid) {
        return AudioResult<void>::ok();
    }
    if (engine == nullptr || source == nullptr) {
        return AudioResult<void>::err(AudioError::InvalidArgument);
    }
    const ma_result r =
        ma_sound_init_copy(engine, source, flags, group, &m_sound);
    if (r != MA_SUCCESS) {
        GAME_LOG_CH_ERROR(channels::kVoice, "ma_sound_init_copy failed: {}",
                          maResultName(r));
        return AudioResult<void>::err(toAudioError(r));
    }
    m_valid = true;
    return AudioResult<void>::ok();
}

void MaSound::uninit() noexcept {
    if (!m_valid) {
        return;
    }
    ma_sound_uninit(&m_sound);
    m_sound = {};
    m_valid = false;
}

bool MaSound::isValid() const noexcept {
    return m_valid;
}

ma_sound* MaSound::get() noexcept {
    return m_valid ? &m_sound : nullptr;
}

const ma_sound* MaSound::get() const noexcept {
    return m_valid ? &m_sound : nullptr;
}

// MaAudioBuffer

MaAudioBuffer::~MaAudioBuffer() {
    uninit();
}

MaAudioBuffer::MaAudioBuffer(MaAudioBuffer&& other) noexcept
    : m_buffer(other.m_buffer), m_valid(other.m_valid) {
    other.m_buffer = {};
    other.m_valid = false;
}

MaAudioBuffer& MaAudioBuffer::operator=(MaAudioBuffer&& other) noexcept {
    if (this != &other) {
        uninit();
        m_buffer = other.m_buffer;
        m_valid = other.m_valid;
        other.m_buffer = {};
        other.m_valid = false;
    }
    return *this;
}

AudioResult<void> MaAudioBuffer::init(const ma_audio_buffer_config* cfg) {
    if (m_valid) {
        return AudioResult<void>::ok();
    }
    if (cfg == nullptr) {
        return AudioResult<void>::err(AudioError::InvalidArgument);
    }
    const ma_result r = ma_audio_buffer_init(cfg, &m_buffer);
    if (r != MA_SUCCESS) {
        GAME_LOG_CH_ERROR(channels::kResource,
                          "ma_audio_buffer_init failed: {}", maResultName(r));
        return AudioResult<void>::err(toAudioError(r));
    }
    m_valid = true;
    return AudioResult<void>::ok();
}

void MaAudioBuffer::uninit() noexcept {
    if (!m_valid) {
        return;
    }
    ma_audio_buffer_uninit(&m_buffer);
    m_buffer = {};
    m_valid = false;
}

bool MaAudioBuffer::isValid() const noexcept {
    return m_valid;
}

ma_audio_buffer* MaAudioBuffer::get() noexcept {
    return m_valid ? &m_buffer : nullptr;
}

const ma_audio_buffer* MaAudioBuffer::get() const noexcept {
    return m_valid ? &m_buffer : nullptr;
}

// MaDecoder

MaDecoder::~MaDecoder() {
    uninit();
}

MaDecoder::MaDecoder(MaDecoder&& other) noexcept
    : m_decoder(other.m_decoder), m_valid(other.m_valid) {
    other.m_decoder = {};
    other.m_valid = false;
}

MaDecoder& MaDecoder::operator=(MaDecoder&& other) noexcept {
    if (this != &other) {
        uninit();
        m_decoder = other.m_decoder;
        m_valid = other.m_valid;
        other.m_decoder = {};
        other.m_valid = false;
    }
    return *this;
}

AudioResult<void> MaDecoder::initFromFile(const char* path,
                                          const ma_decoder_config* cfg) {
    if (m_valid) {
        return AudioResult<void>::ok();
    }
    if (path == nullptr) {
        return AudioResult<void>::err(AudioError::InvalidArgument);
    }
    const ma_result r = ma_decoder_init_file(path, cfg, &m_decoder);
    if (r != MA_SUCCESS) {
        GAME_LOG_CH_ERROR(channels::kResource,
                          "ma_decoder_init_file('{}') failed: {}", path,
                          maResultName(r));
        return AudioResult<void>::err(toAudioError(r));
    }
    m_valid = true;
    return AudioResult<void>::ok();
}

AudioResult<void> MaDecoder::initFromMemory(const void* data, std::size_t size,
                                            const ma_decoder_config* cfg) {
    if (m_valid) {
        return AudioResult<void>::ok();
    }
    if (data == nullptr || size == 0) {
        return AudioResult<void>::err(AudioError::InvalidArgument);
    }
    const ma_result r = ma_decoder_init_memory(data, size, cfg, &m_decoder);
    if (r != MA_SUCCESS) {
        GAME_LOG_CH_ERROR(channels::kResource,
                          "ma_decoder_init_memory failed: {}", maResultName(r));
        return AudioResult<void>::err(toAudioError(r));
    }
    m_valid = true;
    return AudioResult<void>::ok();
}

ma_uint64 MaDecoder::cursorInPcmFrames() const noexcept {
    if (!m_valid) {
        return 0;
    }
    ma_uint64 f = 0;
    auto* ds = const_cast<ma_data_source*>(
        reinterpret_cast<const ma_data_source*>(&m_decoder));
    return ma_data_source_get_cursor_in_pcm_frames(ds, &f) == MA_SUCCESS ? f
                                                                         : 0;
}

ma_result MaDecoder::setLoopPointInPcmFrames(ma_uint64 start,
                                             ma_uint64 end) noexcept {
    if (!m_valid) {
        return MA_INVALID_ARGS;
    }
    auto* ds = reinterpret_cast<ma_data_source*>(&m_decoder);
    return ma_data_source_set_loop_point_in_pcm_frames(ds, start, end);
}

void MaDecoder::clearLoopPoint() noexcept {
    if (!m_valid) {
        return;
    }
    auto* ds = reinterpret_cast<ma_data_source*>(&m_decoder);
    ma_data_source_set_loop_point_in_pcm_frames(ds, 0, 0);
}

ma_data_source* MaDecoder::dataSource() noexcept {
    return m_valid ? reinterpret_cast<ma_data_source*>(&m_decoder) : nullptr;
}

void MaDecoder::uninit() noexcept {
    if (!m_valid) {
        return;
    }
    ma_decoder_uninit(&m_decoder);
    m_decoder = {};
    m_valid = false;
}

bool MaDecoder::isValid() const noexcept {
    return m_valid;
}

ma_decoder* MaDecoder::get() noexcept {
    return m_valid ? &m_decoder : nullptr;
}

const ma_decoder* MaDecoder::get() const noexcept {
    return m_valid ? &m_decoder : nullptr;
}

}  // namespace empire::audio::detail