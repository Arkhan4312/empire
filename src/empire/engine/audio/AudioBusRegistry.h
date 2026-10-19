#pragma once

#include <empire/engine/audio/AudioBus.h>
#include <empire/engine/audio/AudioError.h>

#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>

namespace empire::audio {

// Snapshot of bus target-volumes. Buses not mentioned here are left alone.
struct AudioBusSnapshot {
    std::unordered_map<std::string, float> volumes;
};

// Owns the bus tree. Thread-unsafe (main-thread only).
//
// - ensure() must be called with parent before child.
// - init(engine) / uninit() can be called repeatedly. used across migration
// - volume 0..1. Values outside are clamped.
class AudioBusRegistry {
public:
    AudioBusRegistry();
    ~AudioBusRegistry();
    AudioBusRegistry(const AudioBusRegistry&) = delete;
    AudioBusRegistry& operator=(const AudioBusRegistry&) = delete;

    // Rebuild every bus ma_sound_group against 'engine'.
    AudioResult<void> init(ma_engine* engine);
    void uninit() noexcept;

    // Register the built-in set from AudioBusConfig.h. Idempotent.
    AudioResult<void> initDefaults();

    // Register a bus. If parent is non-empty, it must already exist.
    AudioResult<BusHandle> ensure(std::string_view name,
                                  std::string_view parent = {});

    bool contains(std::string_view name) const noexcept;
    BusHandle find(std::string_view name) const noexcept;

    std::size_t size() const noexcept {
        return m_handles.size();
    }

    // volumes
    void setVolume(BusHandle, float);
    float volume(BusHandle) const;

    void setMuted(BusHandle, bool);
    bool muted(BusHandle) const;

    void setSolo(BusHandle, bool);
    bool solo(BusHandle) const;
    bool anySolo() const noexcept {
        return m_anySolo;
    }
    void clearAllSolo();

    void applySnapshot(const AudioBusSnapshot&, float fadeSec = 0.5f);

    void apply(BusHandle);
    void applyAll();

    void update(float dt);
    AudioResult<void> remove(BusHandle h);
    detail::MaSoundGroup* group(BusHandle) noexcept;

    template <typename F>
    void forEach(F&& f) {
        m_handles.forEach(std::forward<F>(f));
    }
    template <typename F>
    void forEach(F&& f) const {
        m_handles.forEach(std::forward<F>(f));
    }

private:
    void recomputeSoloState();

    util::HandleTable<BusTag, AudioBus> m_handles;
    std::unordered_map<std::string, BusHandle> m_byName;
    bool m_anySolo = false;
};
}  // namespace empire::audio