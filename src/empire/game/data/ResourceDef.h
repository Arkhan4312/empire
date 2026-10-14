#pragma once
#include <cstdint>
#include <string>

namespace game {

// Runtime resource id. It's index in Content::allResources.
using ResourceId = std::uint16_t;
inline constexpr ResourceId kInvalidResource = 0xFFFFu;

struct ResourceAmount {
    ResourceId id = kInvalidResource;
    double amount = 0.0;
};

struct ResourceDef {
    std::string id;
    std::string name;
    double clickYield = 0.0;
    double baseRate = 0.0;
};
}  // namespace game