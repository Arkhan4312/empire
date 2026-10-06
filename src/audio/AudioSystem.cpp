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

    bool hasActiveMusic() const noexcept {
        for (const auto& m : music) {
            if (m.initialized && !m.toStop) {
                return true;
            }
        }
        return false;
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
        std::fprintf(stderr, "[Audio] ma_engine_init failes\n");
        return false;
    }
}

void AudioSystem::shutdown() {
}

bool AudioSystem::ready() const noexcept {
    return false;
}

SoundId AudioSystem::loadSound(std::string_view name, const SoundDesc& desc) {
    return SoundId();
}

void AudioSystem::unloadAllSounds() {
}

void AudioSystem::play(SoundId id, float volume, float pitch) {
}

void AudioSystem::playAt(SoundId id, float panX, float volume) {
}

void AudioSystem::playMusic(std::string_view path, float fadeInSec, bool loop) {
}

void AudioSystem::stopMusic(float fadeOutSec) {
}

void AudioSystem::crossfadeTo(std::string_view path, float durationSec,
                              bool loop) {
}

void AudioSystem::setMasterVolume(float v) {
}

void AudioSystem::setSfxVolume(float v) {
}

void AudioSystem::setMusicVolume(float v) {
}

float AudioSystem::masterVolume() const noexcept {
    return 0.0f;
}

float AudioSystem::sfxVolume() const noexcept {
    return 0.0f;
}

float AudioSystem::musicVolume() const noexcept {
    return 0.0f;
}

void AudioSystem::update(float dt) {
}

void AudioSystem::setPaused(bool paused) {
}

bool AudioSystem::paused() const noexcept {
    return false;
}

}  // namespace game::audio