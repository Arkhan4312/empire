#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

namespace game::audio {

    using SoundId = std::uint32_t;
    inline constexpr SoundId kInvalidSound = 0u;

    struct SoundDesc {
        std::string path;
        float baseVolume = 1.0f;
        bool loop = false;
    };

    class AudioSystem {
    public:
        AudioSystem();
        ~AudioSystem();
        AudioSystem(const AudioSystem&) = delete;
        AudioSystem& operator=(const AudioSystem&) = delete;

        bool init(); // create device+engine; false if unavailable
        void shutdown();
        bool ready() const noexcept;

        // Sfx 
        SoundId loadSound(std::string_view name, const SoundDesc& desc);
        void unloadAllSounds();

        void play(SoundId id, float volume = 1.0f, float pitch = 1.0f);
        void playAt(SoundId id, float panX, float volume = 1.0f);

        void playMusic(std::string_view path, float fadeInSec = 0.8f, bool loop = true);
        void stopMusic(float fadeOutSec = 0.8f);
        void crossfadeTo(std::string_view path, float durationSec = 1.2f, bool loop = true);

        void setMasterVolume(float v);
        void setSfxVolume(float v);
        void setMusicVolume(float v);
        float masterVolume() const noexcept;
        float sfxVolume() const noexcept;
        float musicVolume() const noexcept;

        void update(float dt);

        void setPaused(bool paused);
        bool paused() const noexcept;

    private:
        struct Impl;
        Impl* m_impl = nullptr;
    };
}