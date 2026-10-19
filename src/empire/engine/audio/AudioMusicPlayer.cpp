#include <empire/engine/audio/AudioBusRegistry.h>
#include <empire/engine/audio/AudioChannels.h>
#include <empire/engine/audio/AudioDeviceManager.h>
#include <empire/engine/audio/AudioEventLog.h>
#include <empire/engine/audio/AudioMusicPlayer.h>
#include <empire/engine/audio/detail/MaRaii.h>
#include <empire/engine/audio/detail/MaResult.h>
#include <empire/engine/log/Logger.h>
#include <empire/engine/util/AssetPath.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <deque>
#include <utility>

namespace empire::audio {

namespace {
constexpr float kHalfPi = 1.57079632679;
inline float equalPowerIn(float t) noexcept {
    if (t <= 0.0f) {
        return 0.0f;
    }
    if (t >= 1.0f) {
        return 1.0f;
    }
    return std::sin(t * kHalfPi);
}
inline float equalPowerOut(float t) noexcept {
    if (t <= 0.0f) {
        return 1.0f;
    }
    if (t >= 1.0f) {
        return 0.0f;
    }
    return std::cos(t * kHalfPi);
}
enum class SlotState : std::uint8_t {
    Free,
    FadingIn,
    Playing,
    FadingOut,
    PausingOut,
    Paused,
};
}  // namespace

struct AudioMusicPlayer::Impl {
    AudioEventLog* events;
    struct Slot {
        detail::MaDecoder decoder;
        detail::MaSound sound;

        SlotState state = SlotState::Free;
        bool loop = true;
        std::string path;

        float fadeStartVolume = 0.0f;
        float targetVolume = 0.0f;
        float currentVolume = 0.0f;
        float fadeElapsed = 0.0f;
        float fadeDuration = 0.0f;
        bool isCrossfade = false;

        ma_uint64 loopStart = 0;
        ma_uint64 loopEnd = 0;

        std::atomic<bool> finished{false};
    };

    struct PendingTrack {
        std::string path;
        float fadeInSec = kDefaultFadeIn;
        bool loop = true;
    };

    AudioDeviceManager* device = nullptr;
    AudioBusRegistry* buses = nullptr;

    std::array<Slot, 2> slots;
    std::deque<PendingTrack> queue;

    std::string currentPath;
    bool musicPaused = false;

    ma_uint64 nextLoopStart = 0;
    ma_uint64 nextLoopEnd = 0;

    // Helpers

    bool ready() const noexcept {
        return device && device->engine() != nullptr;
    }

    ma_sound_group* group() const noexcept {
        if (!buses) {
            return nullptr;
        }
        BusHandle h = buses->find("music");
        if (!h.isValid()) {
            h = buses->find("master");
        }
        auto* g = buses->group(h);
        return g ? g->get() : nullptr;
    }

    void destroySlotContents(Slot& s) {
        s.sound.uninit();
        s.decoder.uninit();
        s.state = SlotState::Free;
        s.path.clear();
        s.currentVolume = 0.0f;
        s.targetVolume = 0.0f;
        s.fadeStartVolume = 0.0f;
        s.fadeElapsed = 0.0f;
        s.fadeDuration = 0.0f;
        s.isCrossfade = false;
        s.finished.store(false, std::memory_order_relaxed);
    }

    static void endCallbackTrampoline(void* userdata, ma_sound*) {
        if (!userdata) {
            return;
        }
        auto* s = static_cast<Slot*>(userdata);
        s->finished.store(true, std::memory_order_release);
    }

    bool startSlotIn(Slot& s, const std::string& path, float fadeInSec,
                     bool loop, bool crossfadeIncoming, ma_uint64 loopStart,
                     ma_uint64 loopEnd) {
        destroySlotContents(s);

        ma_engine* eng = device->engine();
        ma_sound_group* grp = group();
        if (!eng || !grp) {
            return false;
        }
        ma_decoder_config dcfg = ma_decoder_config_init(ma_format_f32, 0, 0);
        const std::string resolved = empire::asset::toString(path);
        auto rd = s.decoder.initFromFile(resolved.c_str(), &dcfg);
        if (!rd.isOk()) {
            GAME_LOG_CH_ERROR(channels::kMusic,
                              "music decoder init failed '{}': {}", path,
                              describe(rd.error()));
            return false;
        }

        if (loop && loopEnd > loopStart) {
            const ma_result lr =
                s.decoder.setLoopPointInPcmFrames(loopStart, loopEnd);
            if (lr != MA_SUCCESS) {
                GAME_LOG_CH_WARN(channels::kMusic,
                                 "set loop-points failed '{}': {}", path,
                                 detail::maResultName(lr));
            }
        }
        ma_data_source* ds = s.decoder.dataSource();
        if (ds == nullptr) {
            destroySlotContents(s);
            return false;
        }

        auto rs =
            s.sound.initFromDataSource(eng, ds, MA_SOUND_FLAG_STREAM, grp);
        if (!rs.isOk()) {
            GAME_LOG_CH_ERROR(channels::kMusic,
                              "music sound init failed '{}': {}", path,
                              describe(rs.error()));
            destroySlotContents(s);
            return false;
        }

        ma_sound_set_looping(s.sound.get(), loop ? MA_TRUE : MA_FALSE);
        ma_sound_set_end_callback(s.sound.get(), &endCallbackTrampoline, &s);
        ma_sound_set_volume(s.sound.get(), 0.0f);

        s.path = path;
        s.loop = loop;
        s.loopStart = loopStart;
        s.loopEnd = loopEnd;
        s.fadeStartVolume = 0.0f;
        s.currentVolume = 0.0f;
        s.targetVolume = 1.0f;
        s.fadeElapsed = 0.0f;
        s.fadeDuration = std::max(0.0f, fadeInSec);
        s.isCrossfade = crossfadeIncoming;
        s.finished.store(false, std::memory_order_relaxed);

        if (fadeInSec <= 0.0f) {
            s.currentVolume = 1.0f;
            ma_sound_set_volume(s.sound.get(), 1.0f);
            s.state = SlotState::Playing;
        } else {
            s.state = SlotState::FadingIn;
        }
        const ma_result sr = ma_sound_start(s.sound.get());
        if (sr != MA_SUCCESS) {
            GAME_LOG_CH_ERROR(channels::kMusic,
                              "ma_sound_start failed '{}': {}", path,
                              detail::maResultName(sr));
            destroySlotContents(s);
            return false;
        }
        return true;
    }

    bool beginPlay(const std::string& path, float fadeSec, bool loop,
                   bool crossfade) {
        for (auto& s : slots) {
            if (s.state == SlotState::Free || s.state == SlotState::FadingOut) {
                continue;
            }
            if (s.path != path) {
                continue;
            }
            if (s.state == SlotState::Paused && s.sound.isValid()) {
                ma_sound_start(s.sound.get());
                if (fadeSec <= 0.0f) {
                    s.state = SlotState::Playing;
                    s.currentVolume = 1.0f;
                    ma_sound_set_volume(s.sound.get(), 1.0f);
                } else {
                    s.state = SlotState::FadingIn;
                    s.fadeStartVolume = 0.0f;
                    s.targetVolume = 1.0f;
                    s.fadeElapsed = 0.0f;
                    s.fadeDuration = fadeSec;
                    s.isCrossfade = false;
                }
                musicPaused = false;
            }
            return true;
        }

        Slot* target = nullptr;
        for (auto& s : slots) {
            if (s.state == SlotState::Free) {
                target = &s;
                break;
            }
        }
        if (!target) {
            for (auto& s : slots) {
                if (s.state == SlotState::FadingOut) {
                    target = &s;
                    break;
                }
            }
        }
        if (!target) {
            target = &slots[0];
        }
        for (auto& s : slots) {
            if (&s == target) {
                continue;
            }
            if (s.state == SlotState::Free || s.state == SlotState::Paused ||
                s.state == SlotState::FadingOut) {
                continue;
            }
            s.state = SlotState::FadingOut;
            s.fadeStartVolume = s.currentVolume;
            s.fadeElapsed = 0.0f;
            s.fadeDuration = std::max(0.01f, fadeSec);
            s.isCrossfade = crossfade;
        }

        if (!startSlotIn(*target, path, fadeSec, loop, crossfade, nextLoopStart,
                         nextLoopEnd)) {
            if (events) {
                events->push(AudioEventKind::ResourceLoadFailed,
                             AudioError::ResourceNotFound,
                             AudioEventLog::hashStr(path), 1);
            }
            return false;
        }

        currentPath = path;
        musicPaused = false;
        return true;
    }

    void onFadeComplete(Slot& s) {
        switch (s.state) {
            case SlotState::FadingIn:
                s.state = SlotState::Playing;
                s.currentVolume = s.targetVolume;
                ma_sound_set_volume(s.sound.get(), s.currentVolume);
                break;
            case SlotState::FadingOut:
                destroySlotContents(s);
                break;
            case SlotState::PausingOut:
                ma_sound_stop(s.sound.get());
                s.currentVolume = 0.0f;
                s.state = SlotState::Paused;
                break;
            default:
                break;
        }
    }

    void updateSlot(Slot& s, float dt) {
        if (s.state == SlotState::Free || s.state == SlotState::Paused) return;
        if (s.fadeDuration <= 0.0f) return;

        s.fadeElapsed += dt;
        float t = s.fadeElapsed / s.fadeDuration;
        if (t > 1.0f) t = 1.0f;

        switch (s.state) {
            case SlotState::FadingIn:
                s.currentVolume = s.isCrossfade
                                      ? s.targetVolume * equalPowerIn(t)
                                      : s.targetVolume * t;
                break;
            case SlotState::FadingOut:
                s.currentVolume = s.isCrossfade
                                      ? s.fadeStartVolume * equalPowerOut(t)
                                      : s.fadeStartVolume * (1.0f - t);
                break;
            case SlotState::PausingOut:
                s.currentVolume = s.fadeStartVolume * (1.0f - t);
                break;
            default:
                return;
        }

        ma_sound_set_volume(s.sound.get(), s.currentVolume);

        if (t >= 1.0f) {
            s.fadeDuration = 0.0f;
            onFadeComplete(s);
        }
    }

    void drainQueue() {
        if (musicPaused || queue.empty()) return;
        for (auto& s : slots) {
            if (s.state == SlotState::Free && s.state == SlotState::Paused) {
                continue;  // something active; wait
            }
            return;
        }
        for (auto& s : slots) {
            if (s.state == SlotState::Paused) {
                destroySlotContents(s);
            }
        }

        auto next = std::move(queue.front());
        queue.pop_front();
        beginPlay(next.path, next.fadeInSec, next.loop, /*crossfade=*/true);
    }
};

// Audio music player

AudioMusicPlayer::AudioMusicPlayer() : m_impl(std::make_unique<Impl>()) {
}

AudioMusicPlayer::~AudioMusicPlayer() {
    uninit();
}

AudioResult<void> AudioMusicPlayer::init(AudioDeviceManager& device,
                                         AudioBusRegistry& buses) {
    m_impl->device = &device;
    m_impl->buses = &buses;
    return AudioResult<void>::ok();
}

void AudioMusicPlayer::uninit() noexcept {
    if (!m_impl) {
        return;
    }
    for (auto& s : m_impl->slots) {
        m_impl->destroySlotContents(s);
    }
    m_impl->queue.clear();
    m_impl->currentPath.clear();
    m_impl->musicPaused = false;
}
// Playback

void AudioMusicPlayer::play(std::string_view path, float fadeInSec, bool loop) {
    if (path.empty() || !m_impl || !m_impl->ready()) {
        return;
    }
    m_impl->beginPlay(std::string(path), fadeInSec, loop, /*crossfade*/ true);
}

void AudioMusicPlayer::playNext(std::string_view path, float fadeInSec,
                                bool loop) {
    if (path.empty() || !m_impl) {
        return;
    }
    m_impl->queue.push_back({std::string(path), fadeInSec, loop});
}

void AudioMusicPlayer::crossfadeTo(std::string_view path, float durationSec,
                                   bool loop) {
    if (path.empty() || !m_impl || !m_impl->ready()) {
        return;
    }
    m_impl->beginPlay(std::string(path), durationSec, loop, true);
}

void AudioMusicPlayer::stop(float fadeOutSec) {
    if (!m_impl) {
        return;
    }
    for (auto& s : m_impl->slots) {
        if (s.state == SlotState::Free) {
            continue;
        }
        if (s.state == SlotState::Paused) {
            m_impl->destroySlotContents(s);
            continue;
        }
        if (s.state == SlotState::FadingOut) {
            continue;
        }
        if (fadeOutSec <= 0.0f) {
            if (s.path == m_impl->currentPath) {
                m_impl->currentPath.clear();
            }
            m_impl->destroySlotContents(s);
            continue;
        }
        s.state = SlotState::FadingOut;
        s.fadeStartVolume = s.currentVolume;
        s.fadeElapsed = 0.0f;
        s.fadeDuration = fadeOutSec;
        s.isCrossfade = false;
    }
    m_impl->currentPath.clear();
    m_impl->queue.clear();
    m_impl->musicPaused = false;
}

void AudioMusicPlayer::clearQueue() noexcept {
    if (m_impl) {
        m_impl->queue.clear();
    }
}

std::size_t AudioMusicPlayer::queuedCount() const noexcept {
    return m_impl ? m_impl->queue.size() : 0;
}

// Pause / Resume

void AudioMusicPlayer::pause(float fadeOutSec) {
    if (!m_impl || m_impl->musicPaused) {
        return;
    }
    m_impl->musicPaused = true;
    for (auto& s : m_impl->slots) {
        if (s.state == SlotState::Free || s.state == SlotState::Paused) {
            continue;
        }
        if (s.state == SlotState::FadingOut) {
            continue;
        }
        if (fadeOutSec <= 0.0f) {
            ma_sound_stop(s.sound.get());
            s.currentVolume = 0.0f;
            s.state = SlotState::Paused;
            continue;
        }
        s.state = SlotState::PausingOut;
        s.fadeStartVolume = s.currentVolume;
        s.fadeElapsed = 0.0f;
        s.fadeDuration = fadeOutSec;
    }
}

void AudioMusicPlayer::resume(float fadeInSec) {
    if (!m_impl || !m_impl->musicPaused) {
        return;
    }
    m_impl->musicPaused = false;
    for (auto& s : m_impl->slots) {
        if (s.state != SlotState::Paused) {
            continue;
        }
        if (!s.sound.isValid()) {
            continue;
        }
        ma_sound_start(s.sound.get());
        if (fadeInSec <= 0.0f) {
            s.state = SlotState::Playing;
            s.currentVolume = 1.0f;
            ma_sound_set_volume(s.sound.get(), 1.0f);
            continue;
        }
        s.state = SlotState::FadingIn;
        s.fadeStartVolume = 0.0f;
        s.targetVolume = 1.0f;
        s.fadeElapsed = 0.0f;
        s.fadeDuration = fadeInSec;
        s.isCrossfade = false;
    }
}

bool AudioMusicPlayer::isPaused() const noexcept {
    return m_impl && m_impl->musicPaused;
}

// Loop-points

void AudioMusicPlayer::setLoopPoints(ma_uint64 startFrame, ma_uint64 endFrame) {
    if (!m_impl) {
        return;
    }
    m_impl->nextLoopStart = startFrame;
    m_impl->nextLoopEnd = endFrame;
}

void AudioMusicPlayer::clearLoopPoints() {
    if (!m_impl) {
        return;
    }
    m_impl->nextLoopStart = 0;
    m_impl->nextLoopEnd = 0;
}

// Diagnostics

bool AudioMusicPlayer::isPlaying() const noexcept {
    if (!m_impl) {
        return false;
    }
    for (const auto& s : m_impl->slots) {
        if (s.state == SlotState::Playing || s.state == SlotState::FadingIn) {
            return true;
        }
    }
    return false;
}

const std::string& AudioMusicPlayer::currentPath() const noexcept {
    static const std::string kEmpty;
    return m_impl ? m_impl->currentPath : kEmpty;
}

ma_uint64 AudioMusicPlayer::currentFrame() const noexcept {
    if (!m_impl) {
        return 0;
    }
    for (const auto& s : m_impl->slots) {
        if (s.state == SlotState::Free || s.state == SlotState::Paused) {
            continue;
        }
        if (!s.decoder.isValid()) {
            continue;
        }
        return s.decoder.cursorInPcmFrames();
    }
    return 0;
}

void AudioMusicPlayer::update(float dt) {
    if (!m_impl || !m_impl->ready() || dt <= 0.0f) {
        return;
    }

    for (auto& s : m_impl->slots) {
        m_impl->updateSlot(s, dt);
    }

    for (auto& s : m_impl->slots) {
        if (s.state != SlotState::Playing) {
            continue;
        }
        if (!s.finished.load(std::memory_order_acquire)) {
            continue;
        }
        if (s.loop) {
            s.finished.store(false, std::memory_order_relaxed);
            continue;
        }
        if (s.path == m_impl->currentPath) {
            m_impl->currentPath.clear();
        }
        m_impl->destroySlotContents(s);
    }
    m_impl->drainQueue();
}

// Migration

void AudioMusicPlayer::onBeforeMigration() {
    if (!m_impl) {
        return;
    }
    for (auto& s : m_impl->slots) {
        if (s.state == SlotState::Free) {
            continue;
        }
        if (s.sound.isValid()) {
            if (s.state == SlotState::Paused) {
                continue;
            }
            ma_sound_stop(s.sound.get());
            s.sound.uninit();
        }
    }
}

void AudioMusicPlayer::onAfterMigration() {
    if (!m_impl || !m_impl->ready()) {
        return;
    }

    ma_engine* eng = m_impl->device->engine();
    ma_sound_group* grp = m_impl->group();
    if (!eng || !grp) {
        return;
    }

    for (auto& s : m_impl->slots) {
        if (s.state == SlotState::Free) {
            continue;
        }
        if (!s.decoder.isValid()) {
            m_impl->destroySlotContents(s);
            continue;
        }
        if (s.state == SlotState::Paused) {
            continue;
        }

        ma_data_source* ds = s.decoder.dataSource();
        auto r = s.sound.initFromDataSource(eng, ds, 0, grp);
        if (!r.isOk()) {
            GAME_LOG_CH_ERROR(
                channels::kMusic,
                "music slot re-init failed after migration '{}': {}", s.path,
                describe(r.error()));
            m_impl->destroySlotContents(s);
            continue;
        }
        ma_sound_set_looping(s.sound.get(), s.loop ? MA_TRUE : MA_FALSE);
        ma_sound_set_end_callback(s.sound.get(), &Impl::endCallbackTrampoline,
                                  &s);
        ma_sound_set_volume(s.sound.get(), s.currentVolume);
        const ma_result sr = ma_sound_start(s.sound.get());
        if (sr != MA_SUCCESS) {
            GAME_LOG_CH_WARN(channels::kMusic,
                             "music resume-after-migration failed '{}': {}",
                             s.path, detail::maResultName(sr));
            m_impl->destroySlotContents(s);
        }
    }
}

}  // namespace empire::audio