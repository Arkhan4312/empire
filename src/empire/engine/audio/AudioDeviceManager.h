#pragma once

#include <empire/engine/audio/AudioError.h>
#include <empire/engine/audio/AudioEventLog.h>
#include <empire/engine/audio/detail/MiniAudio.h>
#include <empire/engine/core/events/EventBus.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace empire::audio {
struct AudioDeviceInfo {
    std::string name;
    bool isDefault = false;
};

enum class MigrationReason {
    Initial,         // first successful init
    CurrentLost,     // current device disappeared
    DefaultChanged,  // system default changed
    UserRequested,   // setOutputDevice()
    FallbackRetry,   // internal retry after a failed migration
};

struct MigrationEvent : public events::Event {
    MigrationReason reason = MigrationReason::Initial;
    int oldDevice = -1;
    int newDevice = -1;
    bool success = false;
    AudioError error = AudioError::NotInitialized;

    MigrationEvent() : Event(events::EventCategory::Audio) {
    }
    MigrationEvent(MigrationReason r, int oldD, int newD, bool ok,
                   AudioError err)
        : Event(events::EventCategory::Audio),
          reason(r),
          oldDevice(oldD),
          newDevice(newD),
          success(ok),
          error(err) {
    }
};

struct DeviceLostEvent : public events::Event {
    int deviceIndex = -1;
    DeviceLostEvent() : Event(events::EventCategory::Audio) {
    }
    explicit DeviceLostEvent(int idx)
        : Event(events::EventCategory::Audio), deviceIndex(idx) {
    }
};
// Owns context + resource manager + engine. Device migration recreates the
// engine but NOT the resource manager - decoded buffers survive. Consumers of
// the engine (bus groups, voice pools, music player) rebuild their miniaudio
// objects via the callbacks below.
// NOTE:: detection of silently-dead backends (device removed from the backend's
// active set without an enumeration change) is NOT performed. The library
// detects only: (a) engine creation failure, (b) device disappearance from
// ma_context_get_devices, (c) default-device change. A backend that dies
// in-place requires an app-level restart or an explicit setOutputDevice() call
// to recover.
class AudioDeviceManager {
public:
    AudioDeviceManager();
    ~AudioDeviceManager();
    AudioDeviceManager(const AudioDeviceManager&) = delete;
    AudioDeviceManager& operator=(const AudioDeviceManager&) = delete;

    // LifeCycle
    // Never throws. Returns error if audio cannot be brought up at all;
    // caller is expected to continue without audio.
    AudioResult<void> init();

    void shutdown() noexcept;
    bool ready() const noexcept;

    // Main-thread. Handles throttled polling + pending migrations.
    void update(float dt);

    // Access (internal to empire::audio)
    ma_context* context() noexcept;
    ma_resource_manager* resourceManager() noexcept;
    ma_engine* engine() noexcept;

    // Device selection. Returns the cached snapshot; refreshed by update().
    const std::vector<AudioDeviceInfo>& listOutputDevices() const noexcept;

    //-1 == follow system default. >=0 == explicit index into the last snapshot.
    // Successful call triggers migration.
    AudioResult<void> setOutputDevice(int index);

    int currentDevice() const noexcept;
    bool followsSystemDefault() const noexcept;

    // Migration hooks. Called on the main thread, before the current engine is
    // torn down. Consumers save any state needed to survive (e.g., music
    // cursor).
    using BeforeMigrationFn = std::function<void()>;

    // Called after the engine has been recreated (include on initial init).
    // Consumers rebuild sound groups / voices against the new engine. Never
    // called if no engine is available.
    using AfterMigrationFn = std::function<void()>;

    void setBeforeMigration(BeforeMigrationFn fn);
    void setAfterMigration(AfterMigrationFn fn);
    void setEventBus(events::EventBus* bus) noexcept;
    // Tuning (tests / advanced)
    void setPollInterval(float seconds);
    float pollInterval() const noexcept;

    void setAutoFollowDefault(bool enabled);
    bool autoFollowDefault() const noexcept;

    void setMigrationFadeDurations(float fadeOutSec, float fadeInSec);
    float migrationFadeOut() const noexcept;
    float migrationFadeIn() const noexcept;

    float migrationGain() const noexcept;

    bool isMigrating() const noexcept;

    void setRetryBackoff(float initialSec, float maxSec);
    float retryBackoffCurrent() const noexcept;

    void setEventLog(AudioEventLog* log) noexcept {
        m_events = log;
    }

private:
    AudioEventLog* m_events = nullptr;
    events::EventBus* m_eventBus = nullptr;
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
}  // namespace empire::audio