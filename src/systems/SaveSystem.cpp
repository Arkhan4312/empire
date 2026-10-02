#include "systems/SaveSystem.h"

#include <chrono>
#include <cmath>
#include <fstream>
#include <nlohmann/json.hpp>

#include "content/Content.h"
#include "core/GameLogic.h"

namespace game {
using nlohmann::json;

static std::int64_t nowSeconds() {
    using namespace std::chrono;
    return duration_cast<seconds>(system_clock::now().time_since_epoch())
        .count();
}
bool SaveSystem::save(const GameState& state, const std::string& path) const {
    json j;
    j["version"] = state.version;
    j["era"] = static_cast<int>(state.era);

    json res;
    for (std::size_t i = 0; i < kResourceCount; ++i) {
        res[resourceName(static_cast<ResourceType>(i))] =
            state.resources.amount[i];
    }
    j["resources"] = res;

    json units = json::array();
    for (const auto& u : state.units) {
        units.push_back({{"id", u.id}, {"count", u.count}});
    }
    j["units"] = units;

    json ups = json::array();
    for (const auto& u : state.upgrades) {
        ups.push_back({{"id", u.id}, {"level", u.level}});
    }

    j["upgrades"] = ups;
    j["bossIndex"] = state.bossIndex;
    j["currentBossHp"] = state.currentBoss.hp;
    j["clickPower"] = state.clickPower;
    j["qualityMult"] = state.qualityMult;
    j["speedMult"] = state.speedMult;
    j["playTimeSeconds"] = state.playTimeSeconds;
    j["rngSeed"] = state.rngSeed;
    j["lastSaveTimestamp"] = state.lastSaveTimestamp;

    std::ofstream out(path);
    if (!out) {
        return false;
    }
    out << j.dump(2);
    return out.good();
}
bool SaveSystem::load(GameState& state, const std::string& path) const {
    std::ifstream in(path);
    if (!in) {
        return false;
    }

    json j;
    try {
        in >> j;
    } catch (...) {
        return false;
    }

    try {
        GameState loaded;
        loaded.version = j.value("version", 1u);
        loaded.era = static_cast<Era>(j.value("era", 1));

        if (j.contains("resources")) {
            const auto& res = j["resources"];
            for (std::size_t i = 0; i < kResourceCount; ++i) {
                const char* name = resourceName(static_cast<ResourceType>(i));
                loaded.resources.amount[i] = res.value(name, 0.0);
            }
        }
        if (j.contains("units")) {
            for (const auto& u : j["units"]) {
                loaded.units.push_back(
                    UnitStack{u.value("id", ""), u.value("count", 0)});
            }
        }
        if (j.contains("upgrades")) {
            for (const auto& u : j["upgrades"]) {
                loaded.upgrades.push_back(
                    UpgradeState{u.value("id", ""), u.value("level", 0)});
            }
        }

        loaded.bossIndex = j.value("bossIndex", 0);
        loaded.clickPower = j.value("clickPower", 1.0);
        loaded.qualityMult = j.value("qualityMult", 1.0);
        loaded.speedMult = j.value("speedMult", 1.0);
        loaded.playTimeSeconds = j.value("playTimeSeconds", (std::int64_t)0);
        loaded.rngSeed = j.value("rngSeed", 0xDEADBEEFCAFEF00DULL);
        loaded.lastSaveTimestamp =
            j.value("lastSaveTimestamp", (std::int64_t)0);

        if (const BossDef* b = content::bossAt(loaded.bossIndex)) {
            loaded.currentBoss =
                Boss{b->id, b->name, b->maxHp, b->maxHp, b->dps};
            loaded.currentBoss.hp = j.value("currentBossHp", b->maxHp);
        }

        state = std::move(loaded);
        return true;
    } catch (...) {
        return false;
    }
}

// offline
std::int64_t SaveSystem::computeOfflineSeconds(const GameState& state,
                                               std::int64_t now) {
    if (now == 0) {
        now = nowSeconds();
    }
    if (state.lastSaveTimestamp <= 0) {
        return 0;
    }
    const std::int64_t diff = now - state.lastSaveTimestamp;
    return diff > 0 ? diff : 0;
}

double SaveSystem::applyOffline(GameState& state, GameLogic& logic,
                                std::int64_t secondsAway) {
    if (secondsAway < 60) {
        return 0.0;
    }

    // cap at 7 days
    constexpr std::int64_t kMaxSeconds = 7LL * 24 * 3600;
    if (secondsAway > kMaxSeconds) {
        secondsAway = kMaxSeconds;
    }

    // speed taper
    struct Segment {
        std::int64_t seconds;
        double speed;
    };
    const Segment segments[] = {{3600, 1.0},
                                {5 * 3600, 0.75},
                                {18 * 3600, 0.5},
                                {6LL * 24 * 3600, 0.25}};

    std::int64_t remaining = secondsAway;
    double simulated = 0.0;

    for (const auto& seg : segments) {
        if (remaining <= 0) {
            break;
        }
        const std::int64_t step = std::min(remaining, seg.seconds);
        const double dt = static_cast<double>(step) * seg.speed;
        logic.tick(state, TickContext{dt, true, seg.speed});
        simulated += dt;
        remaining -= step;
    }

    return simulated;
}
}  // namespace game