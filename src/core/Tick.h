#pragma once
#include <cstdint>

namespace game {
struct TickContext {
    double dt = 0.0;
    bool isOffline = false;
    double offlineSpeed = 1.0;
};
}  // namespace game