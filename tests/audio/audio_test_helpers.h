#pragma once

#include <empire/engine/audio/AudioError.h>
#include <empire/engine/audio/AudioResource.h>
#include <empire/engine/audio/AudioResourceCache.h>
#include <empire/engine/audio/detail/MiniAudio.h>
#include <empire/engine/util/AssetPath.h>

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

namespace empire::audio::test {

namespace fs = std::filesystem;

// -------------------------------------------------------------------------
// Scoped temp dir
// -------------------------------------------------------------------------
class ScopedTempDir {
public:
    ScopedTempDir() {
        std::error_code ec;
        auto base = fs::temp_directory_path(ec);
        if (ec) base = fs::current_path(ec);
        static std::atomic<unsigned> counter{0};
        path_ = base /
                ("empire_audio_test_" + std::to_string(counter.fetch_add(1)));
        fs::remove_all(path_, ec);
        fs::create_directories(path_, ec);
    }
    ~ScopedTempDir() {
        std::error_code ec;
        fs::remove_all(path_, ec);
    }
    const fs::path& path() const noexcept {
        return path_;
    }

    ScopedTempDir(const ScopedTempDir&) = delete;
    ScopedTempDir& operator=(const ScopedTempDir&) = delete;

private:
    fs::path path_;
};

// -------------------------------------------------------------------------
// Scoped AssetPath root
// -------------------------------------------------------------------------
class ScopedAssetRoot {
public:
    explicit ScopedAssetRoot(fs::path p) : prev_(empire::asset::root()) {
        empire::asset::setRoot(std::move(p));
    }
    ~ScopedAssetRoot() {
        empire::asset::setRoot(prev_);
    }

    ScopedAssetRoot(const ScopedAssetRoot&) = delete;
    ScopedAssetRoot& operator=(const ScopedAssetRoot&) = delete;

private:
    fs::path prev_;
};

// -------------------------------------------------------------------------
// WAV generation (16-bit PCM mono/stereo)
// -------------------------------------------------------------------------
inline std::vector<std::byte> makeWavBytes(int channels,
                                           std::uint32_t sampleRate,
                                           double durationSec,
                                           std::int16_t amplitude = 16000) {
    const auto frames =
        static_cast<std::uint32_t>(std::round(sampleRate * durationSec));
    const std::uint16_t bitsPerSample = 16;
    const std::uint32_t dataSize =
        frames * static_cast<std::uint32_t>(channels) * (bitsPerSample / 8);
    const std::uint32_t fileSize = 36 + dataSize;

    std::vector<std::byte> out(44 + dataSize);
    auto writeU32 = [&](std::size_t off, std::uint32_t v) {
        out[off + 0] = static_cast<std::byte>(v & 0xFF);
        out[off + 1] = static_cast<std::byte>((v >> 8) & 0xFF);
        out[off + 2] = static_cast<std::byte>((v >> 16) & 0xFF);
        out[off + 3] = static_cast<std::byte>((v >> 24) & 0xFF);
    };
    auto writeU16 = [&](std::size_t off, std::uint16_t v) {
        out[off + 0] = static_cast<std::byte>(v & 0xFF);
        out[off + 1] = static_cast<std::byte>((v >> 8) & 0xFF);
    };
    auto writeStr = [&](std::size_t off, const char* s) {
        for (int i = 0; i < 4; ++i) out[off + i] = static_cast<std::byte>(s[i]);
    };

    writeStr(0, "RIFF");
    writeU32(4, fileSize - 8);
    writeStr(8, "WAVE");
    writeStr(12, "fmt ");
    writeU32(16, 16);
    writeU16(20, 1);  // PCM
    writeU16(22, static_cast<std::uint16_t>(channels));
    writeU32(24, sampleRate);
    writeU32(28, sampleRate * channels * (bitsPerSample / 8));
    writeU16(32, static_cast<std::uint16_t>(channels * (bitsPerSample / 8)));
    writeU16(34, bitsPerSample);
    writeStr(36, "data");
    writeU32(40, dataSize);

    auto* samples = reinterpret_cast<std::int16_t*>(out.data() + 44);
    constexpr double twoPi = 6.283185307179586;
    for (std::uint32_t f = 0; f < frames; ++f) {
        const double t = static_cast<double>(f) / sampleRate;
        const auto s =
            static_cast<std::int16_t>(amplitude * std::sin(twoPi * 440.0 * t));
        for (int c = 0; c < channels; ++c) samples[f * channels + c] = s;
    }
    return out;
}

inline fs::path writeWav(const fs::path& dir, const std::string& name,
                         int channels, std::uint32_t sampleRate,
                         double durationSec) {
    const auto path = dir / name;
    const auto bytes = makeWavBytes(channels, sampleRate, durationSec);
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    f.write(reinterpret_cast<const char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
    return path;
}

inline fs::path writeFile(const fs::path& dir, const std::string& name,
                          const std::vector<std::byte>& bytes) {
    const auto path = dir / name;
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    f.write(reinterpret_cast<const char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
    return path;
}

inline fs::path writeFile(const fs::path& dir, const std::string& name,
                          const std::string& content) {
    const auto path = dir / name;
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    f.write(content.data(), static_cast<std::streamsize>(content.size()));
    return path;
}

// -------------------------------------------------------------------------
// Polling helper for async completion
// -------------------------------------------------------------------------
template <typename F>
inline bool pollUntil(
    F&& pred, std::chrono::milliseconds timeout = std::chrono::seconds(3)) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (pred()) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return pred();
}

// Blocking wait for one completion matching `logicalPath`.
inline std::vector<AudioResourceCache::CompletedLoad> waitForCompletion(
    AudioResourceCache& cache, std::string_view logicalPath,
    std::chrono::milliseconds timeout = std::chrono::seconds(5)) {
    std::vector<AudioResourceCache::CompletedLoad> out;
    pollUntil(
        [&] {
            auto batch = cache.drainCompletions();
            for (auto& c : batch) {
                if (c.logicalPath == logicalPath) out.push_back(std::move(c));
            }
            return !out.empty();
        },
        timeout);
    return out;
}

// -------------------------------------------------------------------------
// Null-backend engine (miniaudio). Used for integration tests that need a
// real ma_engine but no physical output device.
// -------------------------------------------------------------------------
class NullEngine {
public:
    NullEngine() {
        ma_backend backends[] = {ma_backend_null};
        ma_context_config ctxCfg = ma_context_config_init();
        if (ma_context_init(backends, 1, &ctxCfg, &context_) != MA_SUCCESS) {
            return;
        }
        ma_engine_config ecfg = ma_engine_config_init();
        ecfg.pContext = &context_;
        ecfg.noAutoStart = MA_TRUE;
        if (ma_engine_init(&ecfg, &engine_) != MA_SUCCESS) {
            ma_context_uninit(&context_);
            return;
        }
        ready_ = true;
    }
    ~NullEngine() {
        if (ready_) {
            ma_engine_uninit(&engine_);
            ma_context_uninit(&context_);
        }
    }
    NullEngine(const NullEngine&) = delete;
    NullEngine& operator=(const NullEngine&) = delete;

    bool ready() const noexcept {
        return ready_;
    }
    ma_engine* engine() noexcept {
        return ready_ ? &engine_ : nullptr;
    }

private:
    ma_context context_{};
    ma_engine engine_{};
    bool ready_ = false;
};

}  // namespace empire::audio::test