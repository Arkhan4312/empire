#pragma once

#include <empire/engine/util/Result.h>

#include <string_view>

namespace empire::audio {
enum class AudioError {
    // INIT/DEVICE
    ContextInitFailed,
    ResourceManagerInitFailed,
    EngineInitFailed,
    DeviceEnumerationFailed,
    DeviceNotFound,
    DeviceSwitchFailed,
    DeviceWatcherFailed,
    NoOutputDeviceAvailable,

    // RESOURCE
    FileNotFound,
    FileReadFailed,
    DecodeFailed,
    UnsupportedFormat,
    ResourceAlreadyLoaded,
    ResourceNotFound,
    ResourceNotReady,

    // SOUND/GROUP
    SoundInitFailed,
    SoundNotReady,
    GroupInitFailed,
    GroupNotReady,

    // VOICE/POOL
    VoicePoolExhausted,
    VoiceNotFound,
    InvalidHandle,

    // BUS
    BusNotFound,
    BusAlreadyExists,
    BusCycleDetected,

    // GENERIC
    NotInitialized,
    InvalidArgument,
    OutOfMemory,
    Internal,
    Unknown,

    Miniaudio,
};

std::string_view describe(AudioError e) noexcept;

template <typename T>
using AudioResult = util::Result<T, AudioError>;

}  // namespace empire::audio