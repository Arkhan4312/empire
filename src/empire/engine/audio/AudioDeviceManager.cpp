#include <empire/engine/audio/AudioChannels.h>
#include <empire/engine/audio/AudioDeviceManager.h>
#include <empire/engine/audio/AudioEventLog.h>
#include <empire/engine/audio/detail/MaRaii.h>
#include <empire/engine/audio/detail/MaResult.h>
#include <empire/engine/log/Logger.h>

#include <algorithm>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace empire::audio {
namespace {

const char* migrationReasonName(MigrationReason r) noexcept {
    switch (r) {
        case MigrationReason::Initial:
            return "Initial";
        case MigrationReason::CurrentLost:
            return "CurrentLost";
        case MigrationReason::DefaultChanged:
            return "DefaultChanged";
        case MigrationReason::UserRequested:
            return "UserRequested";
        case MigrationReason::FallbackRetry:
            return "FallbackRetry";
    }
    return "?";
}

int findDeviceIndex(const std::vector<ma_device_info>& list,
                    const ma_device_id& id) noexcept {
    for (std::size_t i = 0; i < list.size(); ++i) {
        if (ma_device_id_equal(&list[i].id, &id)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int findDefaultIndex(const std::vector<ma_device_info>& list) noexcept {
    for (std::size_t i = 0; i < list.size(); ++i) {
        if (list[i].isDefault) {
            return static_cast<int>(i);
        }
    }
    return -1;
}
}  // namespace

// Impl

struct AudioDeviceManager::Impl {
    AudioEventLog* events;
    detail::MaLog log;
    detail::MaContext context;
    detail::MaResourceManager resourceManager;
    detail::MaEngine engine;
    std::vector<ma_device_info> devices;
    std::vector<AudioDeviceInfo> publicDevices;
    enum class Phase : std::uint8_t {
        Idle,
        FadingOut,
        FadingIn,
    };

    Phase phase = Phase::Idle;
    float fadeProgress = 0.0f;
    float fadeOutSec = 0.3f;
    float fadeInSec = 0.2f;
    MigrationReason pendingReason = MigrationReason::Initial;

    bool autoFollowDefault = true;
    float retryBackoffInitial = 1.0f;
    float retryBackoffMax = 30.0f;
    float retryBackoffSec = 1.0f;
    // what we intend to play on
    bool followsDefault = true;
    int explicitIndex = -1;

    // what we are actually playing on.
    ma_device_id currentId{};
    bool hasCurrentId = false;
    int currentIndex = -1;

    // Device the system had as default when we started. Used for recovery
    ma_device_id startupDefaultId{};
    bool hasStartupDefaultId = false;

    // Last observed system default (for change detection)
    ma_device_id lastDefaultId{};
    bool hasLastDefaultId = false;

    float pollIntervalSec = 1.0f;
    float pollAccumSec = 0.0f;

    bool hadSuccessfulEngine = false;  // for logging once

    AudioDeviceManager::BeforeMigrationFn beforeCallback;
    AudioDeviceManager::AfterMigrationFn afterCallback;
    AudioDeviceManager::MigrationCallback migrationCallback;

    bool ready() const noexcept {
        return engine.isValid();
    }

    // Helpers

    AudioResult<void> refreshDeviceList() {
        ma_device_info* pPlayback = nullptr;
        ma_uint32 count = 0;

        const ma_result r = ma_context_get_devices(context.get(), &pPlayback,
                                                   &count, nullptr, nullptr);
        if (r != MA_SUCCESS) {
            GAME_LOG_CH_WARN(channels::kDevice, "device enumeration failed: {}",
                             detail::maResultName(r));
            return AudioResult<void>::err(AudioError::DeviceEnumerationFailed);
        }
        devices.clear();
        if (count > 0 && pPlayback) {
            devices.assign(pPlayback, pPlayback + count);
        }
        return AudioResult<void>::ok();
    }

    // deviceIndex == -1 means "system default, whatever it is now".
    // deviceIndex >= 0 must index into 'devices'.
    AudioResult<void> initEngineWithDevice(int deviceIndex) {
        ma_engine_config ecfg = ma_engine_config_init();
        ecfg.pContext = context.get();
        ecfg.pResourceManager = resourceManager.get();
        ecfg.pLog = log.get();
        ecfg.noAutoStart = MA_TRUE;

        if (deviceIndex >= 0 &&
            deviceIndex < static_cast<int>(devices.size())) {
            ecfg.pPlaybackDeviceID = &devices[deviceIndex].id;
        }

        auto r = engine.init(&ecfg);
        if (!r.isOk()) {
            return r;
        }

        ma_device* dev = ma_engine_get_device(engine.get());
        if (dev != nullptr) {
            ma_device_info info{};
            if (ma_device_get_info(dev, ma_device_type_playback, &info) ==
                MA_SUCCESS) {
                currentId = info.id;
                hasCurrentId = true;
                currentIndex = findDeviceIndex(devices, info.id);

                GAME_LOG_CH_INFO(
                    channels::kDevice,
                    "playing on '{}' (list index {}, requested {})", info.name,
                    currentIndex, deviceIndex);
            } else {
                // Very unusual - engine is up but we can't identify the device.
                // Keep going; we can still detect loss later.
                hasCurrentId = false;
                currentIndex = -1;
                GAME_LOG_CH_WARN(channels::kDevice,
                                 "ma_device_get_info failed after engine init");
            }
        }
        return AudioResult<void>::ok();
    }

    // Try to bring the engine up. Honours followsDefault / explicitIndex, and
    // reason-specific fallback chains.
    AudioResult<void> migrate(MigrationReason reason) {
        const int oldDevice = currentIndex;
        const bool wasReady = engine.isValid();

        if (beforeCallback) {
            beforeCallback();
        }

        // tear down engine, keep context + resource manager
        engine.uninit();
        currentIndex = -1;
        hasCurrentId = false;
        currentId = {};

        AudioResult<void> lastError =
            AudioResult<void>::err(AudioError::DeviceSwitchFailed);

        auto tryInit = [&](int idx) -> bool {
            auto r = initEngineWithDevice(idx);
            if (r.isOk()) {
                return true;
            }
            lastError = std::move(r);
            return false;
        };

        bool success = false;

        if (!followsDefault && explicitIndex >= 0) {
            if (tryInit(explicitIndex)) {
                success = true;
            } else {
                GAME_LOG_CH_WARN(channels::kDevice,
                                 "explicit device {} unavailable ({}), falling "
                                 "back to default",
                                 explicitIndex, describe(lastError.error()));
                followsDefault = true;
                explicitIndex = -1;
            }
        }

        if (!success && followsDefault) {
            // Special recovery case: we lost our current device and had a
            // startup default. try it first, then system default.
            if (reason == MigrationReason::CurrentLost && hasStartupDefaultId) {
                const int idx = findDeviceIndex(devices, startupDefaultId);
                if (idx >= 0) {
                    GAME_LOG_CH_DEBUG(
                        channels::kDevice,
                        "recovery: trying startup default at index {}", idx);
                    success = tryInit(idx);
                }
            }

            if (!success) {
                success = tryInit(-1);
                if (!success) {
                    GAME_LOG_CH_DEBUG(
                        channels::kDevice,
                        "system default unavailable, iterating {} devices",
                        devices.size());
                }
            }

            if (!success) {
                for (std::size_t i = 0; i < devices.size(); ++i) {
                    if (tryInit(static_cast<int>(i))) {
                        success = true;
                        break;
                    }
                }
            }
        }

        // report
        MigrationEvent ev{};
        ev.reason = reason;
        ev.oldDevice = oldDevice;
        ev.newDevice = success ? currentIndex : -1;
        ev.success = success;
        ev.error = success ? AudioError::Unknown : lastError.error();

        if (events) {
            events->push(AudioEventKind::DeviceMigration, ev.error,
                         static_cast<std::uint32_t>(reason),
                         success ? static_cast<std::uint32_t>(currentIndex)
                                 : 0xFFFFFFFFu);
        }
        if (success) {
            GAME_LOG_CH_INFO(
                channels::kDevice, "migration OK: reason={}, old={}, new={}",
                migrationReasonName(reason), oldDevice, currentIndex);
            if (afterCallback) {
                afterCallback();
            } else {
                if (auto* eng = engine.get()) {
                    const ma_result sr = ma_engine_start(eng);
                    if (sr != MA_SUCCESS) {
                        GAME_LOG_CH_ERROR(channels::kEngine,
                                          "ma_engine_start failed: {}",
                                          detail::maResultName(sr));
                    }
                }
            }
            hadSuccessfulEngine = true;
        } else {
            // Log once at ERROR when we just went down, then DEBUG on retries.
            const auto level = (wasReady || hadSuccessfulEngine)
                                   ? log::Level::Error
                                   : log::Level::Debug;
            auto* logger = log::defaultLogger();
            if (logger) {
                logger->log(channels::kDevice, level,
                            std::source_location::current(),
                            "migration FAILED: reason={}, old={}, error={}",
                            migrationReasonName(reason), oldDevice,
                            describe(lastError.error()));
            }
            if (wasReady) {
                hadSuccessfulEngine = false;
            }
        }
        if (migrationCallback) {
            migrationCallback(ev);
        }

        return success ? AudioResult<void>::ok()
                       : AudioResult<void>::err(ev.error);
    }

    float migrationGain() const noexcept {
        switch (phase) {
            case Phase::FadingOut:
                return 1.0f - fadeProgress;
            case Phase::FadingIn:
                return fadeProgress;
            case Phase::Idle:
                return 1.0f;
        }
        return 1.0f;
    }

    void requestMigration(MigrationReason reason) {
        if (phase != Phase::Idle) {
            return;
        }
        pendingReason = reason;

        const bool canFadeOut = engine.isValid() && fadeOutSec > 0.0f &&
                                reason != MigrationReason::CurrentLost &&
                                reason != MigrationReason::Initial &&
                                reason != MigrationReason::FallbackRetry;
        if (canFadeOut) {
            phase = Phase::FadingOut;
            fadeProgress = 0.0f;
            return;
        }

        const auto r = migrate(reason);
        if (r.isOk()) {
            retryBackoffSec = retryBackoffInitial;
            if (fadeInSec > 0.0f && engine.isValid()) {
                phase = Phase::FadingIn;
                fadeProgress = 0.0f;
                if (auto* eng = engine.get()) {
                    ma_engine_set_volume(eng, 0.0f);
                }
            }
        } else {
            retryBackoffSec = std::min(retryBackoffSec * 2.0f, retryBackoffMax);
        }
    }

    void updateMigrationStateMachine(float dt) {
        if (phase == Phase::Idle) {
            return;
        }

        if (phase == Phase::FadingOut) {
            fadeProgress += dt / fadeOutSec;
            if (fadeProgress < 1.0f) {
                return;
            }
            fadeProgress = 1.0f;

            const auto r = migrate(pendingReason);

            if (r.isOk()) {
                retryBackoffSec = retryBackoffInitial;
                if (fadeInSec > 0.0f && engine.isValid()) {
                    phase = Phase::FadingIn;
                    fadeProgress = 0.0f;
                    if (auto* eng = engine.get()) {
                        ma_engine_set_volume(eng, 0.0f);
                    }
                } else {
                    phase = Phase::Idle;
                    if (auto* eng = engine.get()) {
                        ma_engine_set_volume(eng, 1.0f);
                    }
                }
            } else {
                retryBackoffSec =
                    std::min(retryBackoffSec * 2.0f, retryBackoffMax);
                phase = Phase::Idle;
            }
            return;
        }

        fadeProgress += dt / fadeInSec;
        if (fadeProgress >= 1.0f) {
            fadeProgress = 1.0f;
            phase = Phase::Idle;
            if (auto* eng = engine.get()) {
                ma_engine_set_volume(eng, 1.0f);
            }
        }
    }

    void ShutdownAll() noexcept {
        engine.uninit();
        resourceManager.uninit();
        context.uninit();
        log.uninit();

        devices.clear();
        publicDevices.clear();
        currentId = {};
        hasCurrentId = false;
        currentIndex = -1;
        startupDefaultId = {};
        hasStartupDefaultId = false;
        lastDefaultId = {};
        hasLastDefaultId = false;
        hadSuccessfulEngine = false;
    }

    // INIT

    AudioResult<void> init() {
        if (ready()) {
            return AudioResult<void>::ok();
        }
        if (context.isValid()) {
            // previously initialized but engine is down. Try recovery
            // immediately (e.g. init was called twice).
            return migrate(MigrationReason::FallbackRetry);
        }

        auto r = log.init();
        if (!r.isOk()) {
            return r;
        }
        ma_context_config ctxCfg = ma_context_config_init();
        ctxCfg.pLog = log.get();

        r = context.init(nullptr, 0, &ctxCfg);
        if (!r.isOk()) {
            if (events) {
                events->push(AudioEventKind::InitFailed, r.error(), 0, 0);
            }
            log.uninit();
            return r;
        }

        ma_resource_manager_config rmCfg = ma_resource_manager_config_init();
        rmCfg.pLog = log.get();

        r = resourceManager.init(&rmCfg);
        if (!r.isOk()) {
            context.uninit();
            log.uninit();
            return r;
        }

        // Enumerate so we have IDs and know which one the system considers
        // default right now.
        (void)refreshDeviceList();  // failure is non-fatal;

        GAME_LOG_CH_INFO(channels::kDevice, "enumerated {} playback device(s)",
                         devices.size());
        for (std::size_t i = 0; i < devices.size(); ++i) {
            GAME_LOG_CH_INFO(channels::kDevice, " [{}] {}{}", i,
                             devices[i].name,
                             devices[i].isDefault ? " [default]" : "");
        }

        const int defIdx = findDefaultIndex(devices);
        if (defIdx >= 0) {
            startupDefaultId = devices[defIdx].id;
            hasStartupDefaultId = true;
            lastDefaultId = startupDefaultId;
            hasLastDefaultId = true;

            GAME_LOG_CH_INFO(channels::kDevice, "startup default device: '{}'",
                             devices[defIdx].name);
        } else {
            hasStartupDefaultId = false;
            hasLastDefaultId = false;
            GAME_LOG_CH_WARN(channels::kDevice,
                             "no default device reported by backend");
        }

        followsDefault = true;
        explicitIndex = -1;

        // Initial migration. Reason = Initial, so consumer knows thi isn't a
        // recovery.
        auto migResult = migrate(MigrationReason::Initial);

        // Even on failure, we keep context + RM alive so update() can retry.
        // Caller checks ready() to decide whether to use audio.
        return migResult;
    }

    // Update

    void update(float dt) {
        if (!context.isValid()) {
            return;  // context is dead. nothing to do.
        }

        updateMigrationStateMachine(dt);
        if (phase != Phase::Idle) {
            return;
        }

        pollAccumSec += dt;
        if (pollAccumSec < pollIntervalSec) {
            return;
        }
        pollAccumSec = 0.0f;

        // Detect that engine is down (previous migration failed) and try to
        // recover on every poll.
        if (!engine.isValid()) {
            retryBackoffSec -= pollIntervalSec;
            if (retryBackoffSec > 0.0f) {
                return;
            }
            (void)refreshDeviceList();
            requestMigration(MigrationReason::FallbackRetry);
            return;
        }

        (void)refreshDeviceList();

        // Did the device we're on disappear from the list?
        bool currentLost = false;
        if (hasCurrentId) {
            if (findDeviceIndex(devices, currentId) < 0) {
                currentLost = true;
                GAME_LOG_CH_WARN(
                    channels::kDevice,
                    "current device no longer present in enumeration");
            }
        }
        if (currentLost) {
            if (events) {
                events->push(AudioEventKind::DeviceLost,
                             AudioError::NoOutputDeviceAvailable,
                             static_cast<std::uint32_t>(currentIndex), 0);
            }
            GAME_LOG_CH_WARN(channels::kDevice, "current device lost");
            requestMigration(MigrationReason::CurrentLost);
            return;
        }

        // Did the system default change?
        if (autoFollowDefault && followsDefault) {
            const int defIdx = findDefaultIndex(devices);
            if (defIdx >= 0) {
                if (!hasLastDefaultId ||
                    !ma_device_id_equal(&lastDefaultId, &devices[defIdx].id)) {
                    lastDefaultId = devices[defIdx].id;
                    hasLastDefaultId = true;
                    GAME_LOG_CH_DEBUG(channels::kDevice,
                                      "system default changed to '{}'",
                                      devices[defIdx].name);
                    requestMigration(MigrationReason::DefaultChanged);
                }
            } else if (hasLastDefaultId) {
                // Backend stopped reporting a default. Rare; just clear.
                hasLastDefaultId = false;
            }
        }
    }
};

// PUBLIC API

AudioDeviceManager::AudioDeviceManager() : m_impl(std::make_unique<Impl>()) {
}

AudioDeviceManager::~AudioDeviceManager() {
    shutdown();
}

AudioResult<void> AudioDeviceManager::init() {
    return m_impl->init();
}

void AudioDeviceManager::shutdown() noexcept {
    if (m_impl) {
        m_impl->ShutdownAll();
    }
}

bool AudioDeviceManager::ready() const noexcept {
    return m_impl && m_impl->ready();
}

void AudioDeviceManager::update(float dt) {
    if (m_impl) {
        m_impl->update(dt);
    }
}

ma_context* AudioDeviceManager::context() noexcept {
    return m_impl ? m_impl->context.get() : nullptr;
}

ma_resource_manager* AudioDeviceManager::resourceManager() noexcept {
    return m_impl ? m_impl->resourceManager.get() : nullptr;
}

ma_engine* AudioDeviceManager::engine() noexcept {
    return m_impl ? m_impl->engine.get() : nullptr;
}

const std::vector<AudioDeviceInfo>& AudioDeviceManager::listOutputDevices()
    const noexcept {
    static const std::vector<AudioDeviceInfo> kEmpty{};
    return m_impl ? m_impl->publicDevices : kEmpty;
}

AudioResult<void> AudioDeviceManager::setOutputDevice(int index) {
    if (!m_impl || !m_impl->context.isValid()) {
        return AudioResult<void>::err(AudioError::NotInitialized);
    }

    // refresh to make sure the user is choosing from a current snapshot.
    (void)m_impl->refreshDeviceList();

    if (index < -1 || index >= static_cast<int>(m_impl->devices.size())) {
        return AudioResult<void>::err(AudioError::DeviceNotFound);
    }
    if (m_impl->phase != Impl::Phase::Idle) {
        if (m_impl->events) {
            m_impl->events->push(
                AudioEventKind::DeviceMigration, AudioError::DeviceSwitchFailed,
                static_cast<std::uint32_t>(MigrationReason::UserRequested),
                static_cast<std::uint32_t>(index));
        }
        return AudioResult<void>::err(AudioError::DeviceSwitchFailed);
    }

    const bool wantDefault = (index == -1);
    // No-op if nothing changes.
    if (wantDefault && m_impl->followsDefault && m_impl->ready()) {
        return AudioResult<void>::ok();
    }
    if (!wantDefault && !m_impl->followsDefault &&
        m_impl->explicitIndex == index && m_impl->ready()) {
        return AudioResult<void>::ok();
    }

    if (wantDefault) {
        m_impl->followsDefault = true;
        m_impl->explicitIndex = -1;
    } else {
        m_impl->followsDefault = false;
        m_impl->explicitIndex = index;
    }

    GAME_LOG_CH_INFO(channels::kDevice, "setOutputDevice({}) requested", index);
    m_impl->requestMigration(MigrationReason::UserRequested);
    return AudioResult<void>::ok();
}

int AudioDeviceManager::currentDevice() const noexcept {
    return m_impl ? m_impl->currentIndex : -1;
}

bool AudioDeviceManager::followsSystemDefault() const noexcept {
    return m_impl && m_impl->followsDefault;
}

void AudioDeviceManager::setBeforeMigration(BeforeMigrationFn fn) {
    if (m_impl) {
        m_impl->beforeCallback = std::move(fn);
    }
}

void AudioDeviceManager::setAfterMigration(AfterMigrationFn fn) {
    if (m_impl) {
        m_impl->afterCallback = std::move(fn);
    }
}

void AudioDeviceManager::setMigrationCallback(MigrationCallback fn) {
    if (m_impl) {
        m_impl->migrationCallback = std::move(fn);
    }
}

void AudioDeviceManager::setPollInterval(float seconds) {
    if (!m_impl) {
        return;
    }
    if (seconds < 0.1f) {
        seconds = 0.1f;
    }
    if (seconds > 10.0f) {
        seconds = 10.0f;
    }
    m_impl->pollIntervalSec = seconds;
}

float AudioDeviceManager::pollInterval() const noexcept {
    return m_impl ? m_impl->pollIntervalSec : 0.0f;
}

void AudioDeviceManager::setAutoFollowDefault(bool enabled) {
    if (m_impl) {
        m_impl->autoFollowDefault = enabled;
    }
}

bool AudioDeviceManager::autoFollowDefault() const noexcept {
    return m_impl && m_impl->autoFollowDefault;
}

void AudioDeviceManager::setMigrationFadeDurations(float fadeOutSec,
                                                   float fadeInSec) {
    if (m_impl) {
        m_impl->fadeOutSec = std::max(0.0f, fadeOutSec);
        m_impl->fadeInSec = std::max(0.0f, fadeInSec);
    }
}

float AudioDeviceManager::migrationFadeOut() const noexcept {
    return m_impl ? m_impl->fadeOutSec : 0.0f;
}

float AudioDeviceManager::migrationFadeIn() const noexcept {
    return m_impl ? m_impl->fadeInSec : 0.0f;
}

float AudioDeviceManager::migrationGain() const noexcept {
    return m_impl ? m_impl->migrationGain() : 1.0f;
}

bool AudioDeviceManager::isMigrating() const noexcept {
    return m_impl && m_impl->phase != Impl::Phase::Idle;
}

void AudioDeviceManager::setRetryBackoff(float initialSec, float maxSec) {
    if (m_impl) {
        m_impl->retryBackoffInitial = std::max(0.05f, initialSec);
        m_impl->retryBackoffMax = std::max(m_impl->retryBackoffInitial, maxSec);
        m_impl->retryBackoffSec = m_impl->retryBackoffInitial;
    }
}

float AudioDeviceManager::retryBackoffCurrent() const noexcept {
    return m_impl ? m_impl->retryBackoffSec : 0.0f;
}

}  // namespace empire::audio