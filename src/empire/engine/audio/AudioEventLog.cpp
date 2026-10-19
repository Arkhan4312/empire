#include <empire/engine/audio/AudioEventLog.h>

#include <algorithm>

namespace empire::audio {
void AudioEventLog::push(AudioEventKind kind, AudioError err, std::uint32_t a,
                         std::uint32_t b) noexcept {
    const std::uint64_t w =
        m_writeCursor.fetch_add(1, std::memory_order_relaxed);
    const std::uint64_t t = m_total.fetch_add(1, std::memory_order_relaxed);
    AudioEvent& e = m_buf[static_cast<std::size_t>(w % kCapacity)];
    e.tick = t;
    e.kind = kind;
    e.error = err;
    e.a = a;
    e.b = b;
}
std::vector<AudioEvent> AudioEventLog::snapshot() const {
    const std::uint64_t w = m_writeCursor.load(std::memory_order_acquire);
    const std::uint64_t start = (w > kCapacity) ? (w - kCapacity) : 0;
    std::vector<AudioEvent> out;
    out.reserve(static_cast<std::size_t>(w - start));
    for (std::uint64_t i = start; i < w; ++i) {
        out.push_back(m_buf[static_cast<std::size_t>(i % kCapacity)]);
    }
    return out;
}
std::size_t AudioEventLog::size() const noexcept {
    const std::uint64_t w = m_writeCursor.load(std::memory_order_acquire);
    return static_cast<std::size_t>(std::min<std::uint64_t>(w, kCapacity));
}
std::uint64_t AudioEventLog::totalCount() const noexcept {
    return m_total.load(std::memory_order_relaxed);
}
std::uint32_t AudioEventLog::hashStr(std::string_view s) noexcept {
    std::uint32_t h = 2166136261u;
    for (unsigned char c : s) {
        h ^= c;
        h *= 16777619u;
    }
    return h;
}
}  // namespace empire::audio