#pragma once
#include <cstdint>

namespace game {

class Rng {
public:
    explicit Rng(std::uint64_t seed)
        : m_state(seed ? seed : 0x9E3779B97F4A7C15ULL) {
    }

    std::uint64_t next() noexcept {
        std::uint64_t z = (m_state += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }

    double nextDouble() noexcept {
        return static_cast<double>(next() >> 11) * (1.0 / 9007199254740992.0);
    }

private:
    std::uint64_t m_state;
};
}  // namespace game