#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "data/ResourceType.h"

namespace game {
// class for controlling and changing the most important aspects of game (such as era, current stats, etc)
enum class Era : std::uint8_t {
    CHILDHOOD = 1,
    ENTREPRENEUR = 2,
};
struct ResourcePool {
    std::array<double, kResourceCount> amount{};
    std::array<double, kResourceCount> rate{};

    double get(ResourceType t) const noexcept;
    void add(ResourceType t, double v) noexcept;
    bool canAfford(ResourceType t, double v) const noexcept;
    bool spend(ResourceType t, double v) noexcept;
};

struct UnitStack {
    std::string id;
    int count = 0;
};

struct UpgradeState {
    std::string id;
    int level = 0;
};

struct Boss {
    std::string id;
    std::string name;
    double maxHp = 100.0;
    double hp = 100.0;
    double dps = 1.0;
};

struct GameState {
    std::uint32_t version = 1;
    Era era = Era::CHILDHOOD;

    ResourcePool resources;
    std::vector<UnitStack> units;
    std::vector<UpgradeState> upgrades;

    int bossIndex = 0;
    Boss currentBoss{};

    double clickPower = 1.0;
    double qualityMult = 1.0;
    double speedMult = 1.0;

    std::int64_t playTimeSeconds = 0;
    std::int64_t lastSaveTimestamp = 0;
    std::uint64_t rngSeed = 0xDEADBEEFCAFEF00DULL;

    // Lookups
    int unitCount(const std::string& id) const noexcept;
    int upgradeLevel(const std::string& id) const noexcept;

    UnitStack* findUnit(const std::string& id) noexcept;
    UpgradeState* findUpgrade(const std::string& id) noexcept;
    const UpgradeState* findUpgrade(const std::string& id) const noexcept;
};

}  // namespace game