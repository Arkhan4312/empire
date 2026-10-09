#include "audio/AudioSystem.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "miniaudio.h"

namespace game::audio {
namespace {
constexpr std::size_t kMaxVoicesPerSfx = 8;
constexpr float kVolumeEpsilon = 1.0e-4f;
}  // namespace

struct AudioSystem::Impl {
    ma_engine engine{};
    bool engineInited = false;

    ma_sound_group masterGroup{};
    ma_sound_group sfxGroup{};
    ma_sound_group musicGroup{};

    struct SfxVoice {
        ma_sound sound{};
        bool initialized = false;
    };
    struct SfxEntry {
        std::string path;
        float baseVolume = 1.0f;
        bool loop = false;
        std::vector<std::unique_ptr<SfxVoice>> voices;
        std::size_t roundRobin = 0;
    };
    std::vector<SfxEntry> sfxByName;
    std::unordered_map<std::string, SoundId> sfxIdMap;

    struct MusicVoice {
        ma_sound sound{};
        bool initialized = false;
        bool toStop = false;
        float currentVolume = 0.0f;
        float targetVolume = 0.0f;
        float startVolume = 0.0f;
        float fadeSeconds = 0.0f;
        float fadeElapsed = 0.0f;
        std::string path;
    };
    std::array<MusicVoice, 2> music;

    float masterV = 1.0f;
    float sfxV = 1.0f;
    float musicV = 1.0f;
    bool isPaused = false;

    void applyVolumes() {
        if (!engineInited) {
            return;
        }
        ma_sound_group_set_volume(&masterGroup, masterV);
        ma_sound_group_set_volume(&sfxGroup, sfxV);
        ma_sound_group_set_volume(&musicGroup, musicV);
    }

    void forceStopMusic(MusicVoice& m) {
        if (m.initialized) {
            ma_sound_stop(&m.sound);
            ma_sound_uninit(&m.sound);
        }
        m = {};
    }

    bool startMusicInSlot(MusicVoice& m, const std::string& path,
                          float fadeInSec, bool loop) {
        forceStopMusic(m);

        const ma_uint32 flags = MA_SOUND_FLAG_STREAM;
        if (ma_sound_init_from_file(&engine, path.c_str(), flags, &musicGroup,
                                    nullptr, &m.sound) != MA_SUCCESS) {
            std::fprintf(stderr, "[Audio] music init failed: %s\n",
                         path.c_str());
            m = {};
            return false;
        }
        m.initialized = true;
        m.toStop = false;
        m.currentVolume = 0.0f;
        m.targetVolume = 1.0f;
        m.startVolume = 0.0f;
        m.fadeElapsed = 0.0f;
        m.fadeSeconds = fadeInSec > 0.0f ? fadeInSec : 0.0f;
        m.path = path;

        ma_sound_set_volume(&m.sound, 0.0f);
        ma_sound_set_looping(&m.sound, loop ? MA_TRUE : MA_FALSE);
        ma_sound_start(&m.sound);
        return true;
    }

    void fadeOutMusic(MusicVoice& m, float durationSec) {
        if (!m.initialized) {
            return;
        }
        m.toStop = true;
        m.startVolume = m.currentVolume;
        m.targetVolume = 0.0f;
        m.fadeElapsed = 0.0f;
        m.fadeSeconds = durationSec > 0.0f ? durationSec : 0.0f;
    }

    bool playSfx(SoundId id, float volume, float pitch, float pan) {
        if (!engineInited) {
            return false;
        }
        if (id == kInvalidSound) {
            return false;
        }
        if (id >= sfxByName.size()) {
            return false;
        }

        auto& e = sfxByName[id];
        if (e.voices.empty()) {
            return false;
        }

        SfxVoice* chosen = nullptr;
        for (auto& v : e.voices) {
            if (v && v->initialized && !ma_sound_is_playing(&v->sound)) {
                chosen = v.get();
                break;
            }
        }

        if (!chosen) {
            const std::size_t n = e.voices.size();
            chosen = e.voices[e.roundRobin % n].get();
            e.roundRobin = (e.roundRobin + 1) % n;
            ma_sound_stop(&chosen->sound);
        }
        if (!chosen || !chosen->initialized) {
            return false;
        }

        ma_sound_set_volume(&chosen->sound, volume * e.baseVolume);
        ma_sound_set_pitch(&chosen->sound, pitch);
        ma_sound_set_pan(&chosen->sound, std::clamp(pan, -1.0f, 1.0f));
        ma_sound_seek_to_pcm_frame(&chosen->sound, 0);
        ma_sound_start(&chosen->sound);
        return true;
    }
};

AudioSystem::AudioSystem() : m_impl(new Impl()) {
}

AudioSystem::~AudioSystem() {
    shutdown();
    delete m_impl;
    m_impl = nullptr;
}

bool AudioSystem::init() {
    if (!m_impl) {
        return false;
    }
    if (m_impl->engineInited) {
        return true;
    }
    ma_engine_config cfg = ma_engine_config_init();
    if (ma_engine_init(&cfg, &m_impl->engine) != MA_SUCCESS) {
        std::fprintf(stderr, "[Audio] ma_engine_init failed\n");
        return false;
    }
    if (ma_sound_group_init(&m_impl->engine, 0, nullptr,
                            &m_impl->masterGroup) != MA_SUCCESS) {
        std::fprintf(stderr, "[Audio] master group init failed\n");
        ma_engine_uninit(&m_impl->engine);
        return false;
    }
    if (ma_sound_group_init(&m_impl->engine, 0, &m_impl->masterGroup,
                            &m_impl->sfxGroup) != MA_SUCCESS) {
        std::fprintf(stderr, "[Audio] sfx group init failde\n");
        ma_sound_group_uninit(&m_impl->masterGroup);
        ma_engine_uninit(&m_impl->engine);
        return false;
    }
    if (ma_sound_group_init(&m_impl->engine, 0, &m_impl->masterGroup,
                            &m_impl->musicGroup) != MA_SUCCESS) {
        std::fprintf(stderr, "[Audio] music group init failed\n");
        ma_sound_group_uninit(&m_impl->masterGroup);
        ma_sound_group_uninit(&m_impl->sfxGroup);
        ma_engine_uninit(&m_impl->engine);
        return false;
    }
    m_impl->engineInited = true;
    m_impl->applyVolumes();
    return true;
}

void AudioSystem::shutdown() {
    if (!m_impl) {
        return;
    }
    if (!m_impl->engineInited) {
        return;
    }
    // Music
    for (auto& m : m_impl->music) {
        if (m.initialized) {
            ma_sound_stop(&m.sound);
            ma_sound_uninit(&m.sound);
        }
        m = {};
    }
    // sfx
    for (auto& s : m_impl->sfxByName) {
        for (auto& v : s.voices) {
            if (v && v->initialized) {
                ma_sound_stop(&v->sound);
                ma_sound_uninit(&v->sound);
            }
        }
        s.voices.clear();
    }

    m_impl->sfxByName.clear();
    m_impl->sfxIdMap.clear();

    ma_sound_group_uninit(&m_impl->musicGroup);
    ma_sound_group_uninit(&m_impl->sfxGroup);
    ma_sound_group_uninit(&m_impl->masterGroup);

    ma_engine_uninit(&m_impl->engine);
    m_impl->engineInited = false;
}

bool AudioSystem::ready() const noexcept {
    return m_impl && m_impl->engineInited;
}

// sfx

SoundId AudioSystem::loadSound(std::string_view name, const SoundDesc& desc) {
    if (!ready()) {
        return kInvalidSound;
    }
    if (name.empty() || desc.path.empty()) {
        return kInvalidSound;
    }

    auto& I = *m_impl;
    const std::string key(name);

    if (auto it = I.sfxIdMap.find(key); it != I.sfxIdMap.end()) {
        return it->second;
    }

    const SoundId id = static_cast<SoundId>(I.sfxByName.size());
    Impl::SfxEntry e;
    e.path = desc.path;
    e.baseVolume = desc.baseVolume;
    e.loop = desc.loop;

    e.voices.reserve(kMaxVoicesPerSfx);
    for (std::size_t i = 0; i < kMaxVoicesPerSfx; ++i) {
        auto v = std::make_unique<Impl::SfxVoice>();
        const ma_uint32 flags = MA_SOUND_FLAG_DECODE;
        if (ma_sound_init_from_file(&I.engine, e.path.c_str(), flags,
                                    &I.sfxGroup, nullptr,
                                    &v->sound) == MA_SUCCESS) {
            v->initialized = true;
            ma_sound_set_volume(&v->sound, 0.0f);
            if (desc.loop) {
                ma_sound_set_looping(&v->sound, MA_TRUE);
            }
            e.voices.push_back(std::move(v));
        }
    }

    if (e.voices.empty()) {
        std::fprintf(stderr, "[Audio] failed to load sfx '%s' (%s)\n",
                     key.c_str(), desc.path.c_str());
        return kInvalidSound;
    }

    I.sfxByName.push_back(std::move(e));
    I.sfxIdMap[key] = id;
    return id;
}

void AudioSystem::unloadAllSounds() {
    if (!m_impl) {
        return;
    }
    for (auto& e : m_impl->sfxByName) {
        for (auto& v : e.voices) {
            if (v && v->initialized) {
                ma_sound_stop(&v->sound);
                ma_sound_uninit(&v->sound);
            }
        }
        e.voices.clear();
    }
    m_impl->sfxByName.clear();
    m_impl->sfxIdMap.clear();
}

void AudioSystem::play(SoundId id, float volume, float pitch) {
    if (!ready()) {
        return;
    }
    m_impl->playSfx(id, volume, pitch, 0.0f);
}

void AudioSystem::playAt(SoundId id, float panX, float volume) {
    if (!ready()) {
        return;
    }
    m_impl->playSfx(id, volume, 1.0f, panX);
}

// Music
void AudioSystem::playMusic(std::string_view path, float fadeInSec, bool loop) {
    if (!ready()) {
        return;
    }
    if (path.empty()) {
        return;
    }

    for (auto& m : m_impl->music) {
        if (m.initialized && !m.toStop && m.path == path) {
            return;
        }
    }

    Impl::MusicVoice* incoming = nullptr;
    for (auto& m : m_impl->music) {
        if (!m.initialized) {
            incoming = &m;
            break;
        }
    }
    if (incoming) {
        for (auto& m : m_impl->music) {
            if (&m != incoming && m.initialized && !m.toStop) {
                m_impl->fadeOutMusic(m, fadeInSec);
            }
        }
        m_impl->startMusicInSlot(*incoming, std::string(path), fadeInSec, loop);
    } else {
        for (auto& m : m_impl->music) {
            m_impl->forceStopMusic(m);
        }
        m_impl->startMusicInSlot(m_impl->music[0], std::string(path), fadeInSec,
                                 loop);
    }
}

void AudioSystem::stopMusic(float fadeOutSec) {
    if (!ready()) {
        return;
    }
    for (auto& m : m_impl->music) {
        if (m.initialized && !m.toStop) {
            m_impl->fadeOutMusic(m, fadeOutSec);
        }
    }
}

void AudioSystem::crossfadeTo(std::string_view path, float durationSec,
                              bool loop) {
    playMusic(path, durationSec, loop);
}
// volume
void AudioSystem::setMasterVolume(float v) {
    if (!m_impl) {
        return;
    }
    v = std::clamp(v, 0.0f, 1.0f);
    m_impl->masterV = v;
    if (m_impl->engineInited) {
        ma_sound_group_set_volume(&m_impl->masterGroup, v);
    }
}

void AudioSystem::setSfxVolume(float v) {
    if (!m_impl) {
        return;
    }
    v = std::clamp(v, 0.0f, 1.0f);
    m_impl->sfxV = v;
    if (m_impl->engineInited) {
        ma_sound_group_set_volume(&m_impl->sfxGroup, v);
    }
}

void AudioSystem::setMusicVolume(float v) {
    if (!m_impl) {
        return;
    }
    v = std::clamp(v, 0.0f, 1.0f);
    m_impl->musicV = v;
    if (m_impl->engineInited) {
        ma_sound_group_set_volume(&m_impl->musicGroup, v);
    }
}

float AudioSystem::masterVolume() const noexcept {
    return m_impl ? m_impl->masterV : 0.0f;
}

float AudioSystem::sfxVolume() const noexcept {
    return m_impl ? m_impl->sfxV : 0.0f;
}

float AudioSystem::musicVolume() const noexcept {
    return m_impl ? m_impl->musicV : 0.0f;
}
// per-frame
void AudioSystem::update(float dt) {
    if (!ready() || dt <= 0.0f) {
        return;
    }

    for (auto& m : m_impl->music) {
        if (!m.initialized) {
            continue;
        }
        if (std::fabs(m.currentVolume - m.targetVolume) <= kVolumeEpsilon) {
            m.currentVolume = m.targetVolume;
            ma_sound_set_volume(&m.sound, m.currentVolume);
            if (m.toStop && m.currentVolume <= kVolumeEpsilon) {
                m_impl->forceStopMusic(m);
            }
            continue;
        }
        if (m.fadeSeconds <= 0.0f) {
            m.currentVolume = m.targetVolume;
        } else {
            m.fadeElapsed += dt;
            float t = m.fadeElapsed / m.fadeSeconds;
            if (t > 1.0f) {
                t = 1.0f;
            }
            m.currentVolume =
                m.startVolume + (m.targetVolume - m.startVolume) * t;
            if (std::fabs(m.currentVolume - m.targetVolume) <= kVolumeEpsilon) {
                m.currentVolume = m.targetVolume;
            }
        }
        ma_sound_set_volume(&m.sound, m.currentVolume);

        if (m.toStop && m.currentVolume <= kVolumeEpsilon) {
            m_impl->forceStopMusic(m);
        }
    }
}

void AudioSystem::setPaused(bool paused) {
    if (!ready()) {
        return;
    }
    if (paused == m_impl->isPaused) {
        return;
    }
    m_impl->isPaused = paused;
    if (paused) {
        ma_engine_stop(&m_impl->engine);
    } else {
        ma_engine_start(&m_impl->engine);
    }
}

bool AudioSystem::paused() const noexcept {
    return m_impl && m_impl->isPaused;
}

}  // namespace game::audio