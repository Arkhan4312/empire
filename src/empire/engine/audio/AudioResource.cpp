#include <empire/engine/audio/AudioChannels.h>
#include <empire/engine/audio/AudioResource.h>
#include <empire/engine/audio/detail/MaRaii.h>
#include <empire/engine/audio/detail/MaResult.h>
#include <empire/engine/log/Logger.h>
#include <empire/engine/util/AssetPath.h>

#include <cstring>
#include <system_error>
#include <utility>

namespace empire::audio {
namespace {
bool ieq(std::string_view a, std::string_view b) noexcept {
    if (a.size() != b.size()) {
        return false;
    }
    for (std::size_t i = 0; i < a.size(); ++i) {
        char ca = a[i];
        char cb = b[i];
        if (ca >= 'A' && ca <= 'Z') {
            ca = static_cast<char>(ca + 32);
        }
        if (cb >= 'A' && cb <= 'Z') {
            cb = static_cast<char>(cb + 32);
        }
        if (ca != cb) {
            return false;
        }
    }
    return true;
}

bool extensionSuggestStream(std::string_view path) noexcept {
    const auto dot = path.find_last_of('.');
    if (dot == std::string_view::npos) {
        return false;
    }
    const std::string_view ext = path.substr(dot + 1);
    return ieq(ext, "ogg") || ieq(ext, "mp3");
}

LoadMode resolveMode(LoadMode m, std::string_view path) noexcept {
    if (m != LoadMode::Auto) {
        return m;
    }
    return extensionSuggestStream(path) ? LoadMode::Stream : LoadMode::Decode;
}
}  // namespace

AudioResource::~AudioResource() {
    uninit();
}

AudioResource::AudioResource(AudioResource&& other) noexcept
    : m_state(other.m_state.load(std::memory_order_relaxed)),
      m_streamed(other.m_streamed),
      m_logicalPath(std::move(other.m_logicalPath)),
      m_resolvedPath(std::move(other.m_resolvedPath)),
      m_mtime(other.m_mtime),
      m_format(other.m_format),
      m_channels(other.m_channels),
      m_sampleRate(other.m_sampleRate),
      m_frameCount(other.m_frameCount),
      m_pcm(std::move(other.m_pcm)) {
    other.m_state = ResourceState::Unloaded;
    other.m_streamed = false;
}

AudioResource& AudioResource::operator=(AudioResource&& other) noexcept {
    if (this != &other) {
        m_state.store(other.m_state.load(std::memory_order_relaxed),
                      std::memory_order_relaxed);
        m_streamed = other.m_streamed;
        m_logicalPath = std::move(other.m_logicalPath);
        m_resolvedPath = std::move(other.m_resolvedPath);
        m_mtime = other.m_mtime;
        m_format = other.m_format;
        m_channels = other.m_channels;
        m_sampleRate = other.m_sampleRate;
        m_frameCount = other.m_frameCount;
        m_pcm = std::move(other.m_pcm);
        other.m_state = ResourceState::Unloaded;
        other.m_streamed = false;
    }
    return *this;
}

AudioResult<void> AudioResource::load(const AudioResourceDesc& desc) {
    return loadImpl(desc, false);
}

void AudioResource::markLoading() noexcept {
    m_state.store(ResourceState::Loading, std::memory_order_relaxed);
}

AudioResult<void> AudioResource::loadInBackground(
    const AudioResourceDesc& desc) {
    return loadImpl(desc, true);
}

void AudioResource::uninit() noexcept {
    m_pcm.clear();
    m_pcm.shrink_to_fit();
    m_state.store(ResourceState::Unloaded, std::memory_order_release);
    m_streamed = false;
    m_logicalPath.clear();
    m_resolvedPath.clear();
    m_mtime = {};
    m_format = ma_format_unknown;
    m_channels = 0;
    m_sampleRate = 0;
    m_frameCount = 0;
}

AudioResult<void> AudioResource::loadImpl(const AudioResourceDesc& desc,
                                          bool background) {
    m_pcm.clear();
    m_pcm.shrink_to_fit();
    m_streamed = false;
    m_logicalPath.clear();
    m_resolvedPath.clear();
    m_mtime = {};
    m_format = ma_format_unknown;
    m_channels = 0;
    m_sampleRate = 0;
    m_frameCount = 0;
    if (!background) {
        m_state.store(ResourceState::Unloaded, std::memory_order_relaxed);
    }
    if (desc.path.empty()) {
        m_state.store(ResourceState::Failed, std::memory_order_release);
        return AudioResult<void>::err(AudioError::InvalidArgument);
    }

    m_logicalPath = desc.path;
    m_resolvedPath = empire::asset::toString(desc.path);

    std::error_code ec;
    const std::filesystem::path fsPath(m_resolvedPath);
    if (!std::filesystem::exists(fsPath, ec) || ec) {
        GAME_LOG_CH_ERROR(channels::kResource,
                          "resource '{}' not found (resolved '{}')",
                          m_logicalPath, m_resolvedPath);
        m_state.store(ResourceState::Failed, std::memory_order_release);
        return AudioResult<void>::err(AudioError::FileNotFound);
    }
    m_mtime = std::filesystem::last_write_time(fsPath, ec);
    if (ec) {
        m_mtime = {};
        ec.clear();
    }

    const LoadMode mode = resolveMode(desc.mode, m_resolvedPath);
    m_streamed = (mode == LoadMode::Stream);

    detail::MaDecoder decoder;
    const ma_decoder_config dcfg = ma_decoder_config_init(ma_format_f32, 0, 0);
    {
        auto r = decoder.initFromFile(m_resolvedPath.c_str(), &dcfg);
        if (!r.isOk()) {
            GAME_LOG_CH_ERROR(channels::kResource,
                              "decoder init failed for '{}'", m_resolvedPath);
            m_state.store(ResourceState::Failed, std::memory_order_release);
            return r;
        }
    }

    ma_decoder* d = decoder.get();
    m_format = d->outputFormat;
    m_channels = d->outputChannels;
    m_sampleRate = d->outputSampleRate;

    if (m_streamed) {
        ma_uint64 frames = 0;
        (void)ma_decoder_get_length_in_pcm_frames(d, &frames);
        m_frameCount = frames;
    } else {
        constexpr ma_uint64 kChunkFrames = 4096;
        std::vector<float> chunk(static_cast<std::size_t>(kChunkFrames) *
                                 m_channels);
        for (;;) {
            ma_uint64 read = 0;
            const ma_result rr = ma_decoder_read_pcm_frames(
                d, chunk.data(), kChunkFrames, &read);
            if (rr != MA_SUCCESS && rr != MA_AT_END) {
                GAME_LOG_CH_ERROR(channels::kResource,
                                  "decode failed mid-stream for '{}': {}",
                                  m_resolvedPath, detail::maResultName(rr));
                m_pcm.clear();
                m_state.store(ResourceState::Failed, std::memory_order_release);
                return AudioResult<void>::err(detail::toAudioError(rr));
            }
            if (read > 0) {
                const std::size_t floats =
                    static_cast<std::size_t>(read) * m_channels;
                const auto* bytes =
                    reinterpret_cast<const std::byte*>(chunk.data());
                m_pcm.insert(m_pcm.end(), bytes,
                             bytes + floats * sizeof(float));
            }
            if (rr == MA_AT_END || read == 0) {
                break;
            }
        }
        m_frameCount = m_pcm.size() / (sizeof(float) * m_channels);
        if (m_pcm.empty()) {
            GAME_LOG_CH_WARN(channels::kResource,
                             "resource '{}' decoded to 0 frames",
                             m_resolvedPath);
            m_state.store(ResourceState::Failed, std::memory_order_release);
            return AudioResult<void>::err(AudioError::DecodeFailed);
        }
    }

    m_state.store(ResourceState::Ready, std::memory_order_release);
    GAME_LOG_CH_DEBUG(channels::kResource,
                      "loaded '{}' [{}] {}ch {}hz {} frames ({:.2f}s)",
                      m_logicalPath, m_streamed ? "stream" : "decode",
                      m_channels, m_sampleRate, m_frameCount,
                      m_sampleRate ? double(m_frameCount) / m_sampleRate : 0.0);
    return AudioResult<void>::ok();
}
}  // namespace empire::audio