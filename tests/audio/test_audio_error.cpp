#include <empire/engine/audio/AudioError.h>

#include <catch2/catch_test_macros.hpp>
#include <initializer_list>

using namespace empire::audio;

TEST_CASE("AudioError: every enumerator has a description", "[audio][error]") {
    const std::initializer_list<AudioError> all = {
        AudioError::ContextInitFailed,
        AudioError::ResourceManagerInitFailed,
        AudioError::EngineInitFailed,
        AudioError::DeviceEnumerationFailed,
        AudioError::DeviceNotFound,
        AudioError::DeviceSwitchFailed,
        AudioError::DeviceWatcherFailed,
        AudioError::NoOutputDeviceAvailable,
        AudioError::FileNotFound,
        AudioError::FileReadFailed,
        AudioError::DecodeFailed,
        AudioError::UnsupportedFormat,
        AudioError::ResourceAlreadyLoaded,
        AudioError::ResourceNotFound,
        AudioError::ResourceNotReady,
        AudioError::SoundInitFailed,
        AudioError::SoundNotReady,
        AudioError::GroupInitFailed,
        AudioError::GroupNotReady,
        AudioError::VoicePoolExhausted,
        AudioError::VoiceNotFound,
        AudioError::InvalidHandle,
        AudioError::BusNotFound,
        AudioError::BusAlreadyExists,
        AudioError::BusCycleDetected,
        AudioError::NotInitialized,
        AudioError::InvalidArgument,
        AudioError::OutOfMemory,
        AudioError::Internal,
        AudioError::Unknown,
        AudioError::Miniaudio,
    };
    for (auto e : all) {
        const auto s = describe(e);
        REQUIRE_FALSE(s.empty());
        REQUIRE(s != "unrecognized audio error");
    }
}

TEST_CASE("AudioError: unknown enumerator returns fallback", "[audio][error]") {
    REQUIRE(describe(static_cast<AudioError>(999)) ==
            "unrecognized audio error");
}