#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace empire::audio {

struct BusDef {
    std::string_view name;
    std::string_view parent;
    float defaultVolume = 1.0f;
};

inline constexpr std::array<BusDef, 6> kDefaultBuses{{
    {"master", "", 1.00f},
    {"music", "master", 0.60f},
    {"sfx", "master", 1.00f},
    {"ui", "master", 0.90f},
    {"voice", "master", 1.00f},
    {"ambient", "master", 0.50f},
}};

// compile-time validation
constexpr bool validateDefaultBuses() noexcept {
    for (std::size_t i = 0; i < kDefaultBuses.size(); ++i) {
        for (std::size_t j = i + 1; j < kDefaultBuses.size(); ++j) {
            if (kDefaultBuses[i].name == kDefaultBuses[j].name) {
                return false;
            }
        }
        if (!kDefaultBuses[i].parent.empty()) {
            bool parentBefore = false;
            for (std::size_t j = 0; j < i; ++j) {
                if (kDefaultBuses[j].name == kDefaultBuses[i].parent) {
                    parentBefore = true;
                    break;
                }
            }
            if (!parentBefore) {
                return false;
            }
        }
    }
    return true;
}
static_assert(validateDefaultBuses(),
              "kDefaultBuses: duplicate names or unknown parents, or parent "
              "declared after its child");
}  // namespace empire::audio