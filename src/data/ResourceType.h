#pragma once
#include <cstddef>
#include <cstdint>

namespace game {
// Resources definition

enum class ResourceType : std::uint16_t {
    PLASTIC = 0,
    PAPER,
    GLUE,
    COUNT,
};

inline constexpr std::size_t kResourceCount = static_cast<std::size_t>(ResourceType::COUNT);

inline const char* resourceName(ResourceType t) noexcept {
    switch (t) {
        case ResourceType::PLASTIC:
            return "plastic";
        case ResourceType::PAPER:
            return "paper";
        case ResourceType::GLUE:
            return "glue";
        default:
            return "unknown";
    }
}
}  // namespace game