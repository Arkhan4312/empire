#include "core/GameState.h"

#include <algorithm>

namespace game {

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
const UpgradeState* GameState::findUpgrade(
    const std::string& id) const noexcept {
    for (const auto& u : upgrades) {
        if (u.id == id) {
            return &u;
        }
    }
    return nullptr;
}

}  // namespace game