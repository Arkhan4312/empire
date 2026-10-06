#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "data/ResourceDef.h"

namespace game {
// class for controlling and changing the most important aspects of game (such
// as era, current stats, etc)
enum class Era : std::uint8_t {
    CHILDHOOD = 1,
    ENTREPRENEUR = 2,
};
class ResourcePool {
public:
    void reset(std::size_t n) {
        m_amount.assign(n, 0.0);
        m_rate.assign(n, 0.0);
    }

    std::size_t size() const noexcept {
        return m_amount.size();
    }

    double get(ResourceId id) const noexcept {
        return id < m_amount.size() ? m_amount[id] : 0.0;
    }

    double getByIdx(std::size_t i) const noexcept {
        return i < m_amount.size() ? m_amount[i] : 0.0;
    }

    void add(ResourceId id, double v) noexcept {
        if (id >= m_amount.size()) {
            return;
        }
        m_amount[id] += v;
        if (m_amount[id] < 0.0) {
            m_amount[id] = 0.0;
        }
    }

    void addByIdx(std::size_t i, double v) noexcept {
        if (i >= m_amount.size()) {
            return;
        }
        m_amount[i] += v;
        if (m_amount[i] < 0.0) {
            m_amount[i] = 0.0;
        }
    }

    void setByIdx(std::size_t i, double v) noexcept {
        if (i < m_amount.size()) {
            m_amount[i] = v;
        }
    }

    bool canAfford(ResourceId id, double v) const noexcept {
        return get(id) >= v;
    }

    bool spend(ResourceId id, double v) noexcept {
        if (id >= m_amount.size() || m_amount[id] < v) {
            return false;
        }
        m_amount[id] -= v;
        return true;
    }

    double rateAt(std::size_t i) const noexcept {
        return i < m_rate.size() ? m_rate[i] : 0.0;
    }

    void setRate(std::size_t i, double v) noexcept {
        if (i < m_rate.size()) {
            m_rate[i] = v;
        }
    }

    void addRate(std::size_t i, double v) noexcept {
        if (i < m_rate.size()) {
            m_rate[i] += v;
        }
    }

private:
    std::vector<double> m_amount;
    std::vector<double> m_rate;
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