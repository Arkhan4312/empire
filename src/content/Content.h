#pragma once
#include <data/BuildingDef.h>

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "core/GameState.h"
#include "data/BossDef.h"
#include "data/ResourceDef.h"
#include "data/UnitDef.h"
#include "data/UpgradeDef.h"

namespace game::content {
// balance
struct OfflineSegment {
    std::int64_t seconds = 0;
    double speed = 1.0;
};

struct Balance {
    double clickBasePower = 1.0;

    std::int64_t autosaveIntervalSeconds = 60;

    std::int64_t offlineMinSeconds = 60;
    std::int64_t offlineMaxSeconds = 7LL * 24 * 3600;
    double offlineSubTickMaxSeconds = 1.0;
    std::vector<OfflineSegment> offlineSegments;

    int childhoodBossCount = 0;

    static const Balance& defaults();
};

using EffectFn = std::function<void(GameState&, double)>;

void registerEffect(std::string name, EffectFn fn);
bool hasEffect(std::string_view name);

class Content {
public:
    static Content& instance();
    // load all jsons from a directory, return false on error;
    // if error, previous remain active
    bool loadFromDirectory(const std::string& dir, std::string* err = nullptr);
    // Reload from the last directory (hot-reload);
    bool reload(std::string* err = nullptr);
    // whether at least one real (non-fallback) load has succeeded.
    bool loadedFromDisk() const noexcept {
        return m_loadedFromDisk;
    }

    // lookups
    const UnitDef* findUnit(std::string_view id) const;
    const BuildingDef* findBuilding(std::string_view id) const;
    const UpgradeDef* findUpgrade(std::string_view id) const;
    const BossDef* findBoss(std::string_view id) const;
    const ResourceDef* resource(ResourceId id) const;
    ResourceId resourceId(std::string_view id) const;

    const BossDef* bossAt(Era era, int index) const;
    int bossCount(Era era) const;

    const std::vector<UnitDef>& allUnits() const noexcept {
        return m_units;
    }
    const std::vector<BuildingDef>& allBuildings() const noexcept {
        return m_buildings;
    }
    const std::vector<UpgradeDef>& allUpgrades() const noexcept {
        return m_upgrades;
    }
    const std::vector<BossDef>& allBosses() const noexcept {
        return m_bosses;
    }
    const std::vector<ResourceDef>& allResources() const noexcept {
        return m_resources;
    }
    const Balance& balance() const noexcept {
        return m_balance;
    }

private:
    Content();

    void installFallback();

    std::vector<UnitDef> m_units;
    std::vector<BuildingDef> m_buildings;
    std::vector<UpgradeDef> m_upgrades;
    std::vector<BossDef> m_bosses;
    std::vector<ResourceDef> m_resources;
    Balance m_balance;

    std::unordered_map<std::string, std::size_t> m_unitIdx;
    std::unordered_map<std::string, std::size_t> m_buildingIdx;
    std::unordered_map<std::string, std::size_t> m_upgradeIdx;
    std::unordered_map<std::string, std::size_t> m_bossIdx;
    std::unordered_map<std::string, ResourceId> m_resourceIdx;

    std::unordered_map<int, std::vector<std::size_t>> m_bossesByEra;

    std::string m_lastDir;
    bool m_loadedFromDisk = false;

    void rebuildIndices();
    void rebuildEraBuckets();
};

inline const UnitDef* findUnit(std::string_view id) {
    return Content::instance().findUnit(id);
}
inline const UpgradeDef* findUpgrade(std::string_view id) {
    return Content::instance().findUpgrade(id);
}
inline const BossDef* bossAt(int index);
inline std::size_t bossCount();

inline const std::vector<UnitDef>& allUnits() {
    return Content::instance().allUnits();
}
inline const std::vector<UpgradeDef>& allUpgrades() {
    return Content::instance().allUpgrades();
}
inline const std::vector<BossDef>& allBosses() {
    return Content::instance().allBosses();
}

// initialize fresh game
void initNewGame(GameState& state);
}  // namespace game::content