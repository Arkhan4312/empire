#include "core/GameState.h"

#include <algorithm>

namespace game {
double ResourcePool::get(ResourceType t) const noexcept {
    return amount[static_cast<std::size_t>(t)];
}

void ResourcePool::add(ResourceType t, double v) noexcept {
    amount[static_cast<std::size_t>(t)] += v;
    if (amount[static_cast<std::size_t>(t)] < 0.0) {
        amount[static_cast<std::size_t>(t)] = 0.0;
    }
}

bool ResourcePool::canAfford(ResourceType t, double v) const noexcept {
    return get(t) >= v;
}

bool ResourcePool::spend(ResourceType t, double v) noexcept {
    const auto idx = static_cast<std::size_t>(t);
    if (amount[idx] < v) {
        return false;
    }
    amount[idx] -= v;
    return true;
}

int GameState::unitCount(const std::string& id) const noexcept {
    for (const auto& u : units) {
        if (u.id == id) {
            return u.count;
        }
    }
    return 0;
}
int GameState::upgradeLevel(const std::string& id) const noexcept {
    for (const auto& u : upgrades) {
        if (u.id == id) {
            return u.level;
        }
    }
    return 0;
}

UnitStack* GameState::findUnit(const std::string& id) noexcept {
    for (auto& u : units) {
        if (u.id == id) {
            return &u;
        }
    }
    return nullptr;
}
UpgradeState* GameState::findUpgrade(const std::string& id) noexcept {
    for (auto& u : upgrades) {
        if (u.id == id) {
            return &u;
        }
    }
    return nullptr;
}
const UpgradeState* GameState::findUpgrade(const std::string& id) const noexcept {
    for (const auto& u : upgrades) {
        if (u.id == id) {
            return &u;
        }
    }
    return nullptr;
}

}  // namespace game