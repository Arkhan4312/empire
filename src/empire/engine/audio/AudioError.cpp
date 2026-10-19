#include <empire/engine/audio/AudioError.h>

namespace empire::audio {
std::string_view describe(AudioError e) noexcept {
    switch (e) {
            // INIT/DEVICE
        case AudioError::ContextInitFailed:
            return "audio context init failed";
        case AudioError::ResourceManagerInitFailed:
            return "audio resource manager init failed";
        case AudioError::EngineInitFailed:
            return "audio engine init failed";
        case AudioError::DeviceEnumerationFailed:
            return "device enumeration failed";
        case AudioError::DeviceNotFound:
            return "requested device not found";
        case AudioError::DeviceSwitchFailed:
            return "device switch failed";
        case AudioError::DeviceWatcherFailed:
            return "device watcher failed";
        case AudioError::NoOutputDeviceAvailable:
            return "no output device available";

            // RESOURCE
        case AudioError::FileNotFound:
            return "file not found";
        case AudioError::FileReadFailed:
            return "file read failed";
        case AudioError::DecodeFailed:
            return "decode failed";
        case AudioError::UnsupportedFormat:
            return "unsupported audio format";
        case AudioError::ResourceAlreadyLoaded:
            return "resource already loaded";
        case AudioError::ResourceNotFound:
            return "resource not found";
        case AudioError::ResourceNotReady:
            return "resource not ready";

            // SOUND/GROUP
        case AudioError::SoundInitFailed:
            return "sound init failed";
        case AudioError::SoundNotReady:
            return "sound not ready";
        case AudioError::GroupInitFailed:
            return "sound group init failed";
        case AudioError::GroupNotReady:
            return "sound group not ready";

            // VOICE/POOL
        case AudioError::VoicePoolExhausted:
            return "voice pool exhausted";
        case AudioError::VoiceNotFound:
            return "voice not found";
        case AudioError::InvalidHandle:
            return "invalid handle";

            // BUS
        case AudioError::BusNotFound:
            return "bus not found";
        case AudioError::BusAlreadyExists:
            return "bus already exists";
        case AudioError::BusCycleDetected:
            return "bus hierarchy cycle detected";

            // GENERIC
        case AudioError::NotInitialized:
            return "audio system not initialized";
        case AudioError::InvalidArgument:
            return "invalid argument";
        case AudioError::OutOfMemory:
            return "out of memory";
        case AudioError::Internal:
            return "internal error";
        case AudioError::Unknown:
            return "unknown error";

        case AudioError::Miniaudio:
            return "miniaudio error";
    }
    return "unrecognized audio error";
}
}  // namespace empire::audio