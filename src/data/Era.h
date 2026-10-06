#pragma once
#include <cstdint>
#include <string_view>
namespace game {
enum class Era : std::uint8_t {
    CHILDHOOD = 1,
    ENTREPRENEUR = 2,
};

inline constexpr Era kFirstEra = Era::CHILDHOOD;
inline constexpr Era kLastEra = Era::ENTREPRENEUR;

inline constexpr std::string_view eraName(Era e) noexcept {
    switch (e) {
        case Era::CHILDHOOD:
            return "CHILDHOOD";
        case Era::ENTREPRENEUR:
            return "ENTREPRENEUR";
    }
    return "UNKNOWN";
}

inline bool eraFromString(std::string_view s, Era& out) noexcept {
    if (s == "CHILDHOOD") {
        out = Era::CHILDHOOD;
        return true;
    }
    if (s == "ENTREPRENEUR") {
        out = Era::ENTREPRENEUR;
        return true;
    }
    return false;
}

inline bool nextEra(Era current, Era& out) noexcept {
    if (current == Era::CHILDHOOD) {
        out = Era::ENTREPRENEUR;
        return true;
    }
    return false;
}
}  // namespace game