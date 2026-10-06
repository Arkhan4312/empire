#include "SaveSystem.h"

#include <chrono>
#include <cmath>
#include <fstream>
#include <nlohmann/json.hpp>
#include <unordered_set>

#include "SaveSystem.h"
#include "content/Content.h"
#include "core/GameLogic.h"
#include "systems/SaveSystem.h"

namespace game {
using nlohmann::json;

namespace {

constexpr std::uint32_t kCurrentSaveVersion = 1;

bool isFiniteNonNegative(double v) noexcept {
    return std::isfinite(v) && v >= 0.0;
}

// Refresh everything that can be recomputed from Content and clamp any values
// that could have been tampered with / corrupted / come from an older balance
// patch.
void sanitize(GameState& s) {
    const auto& C = content::Content::instance();
    // Resources
    const std::size_t n = C.allResources().size();
    if (s.resources.size() != n) {
        const auto oldSize = s.resources.size();
        const std::size_t keep = std::min(oldSize, n);
        std::vector<double> tmp(keep);
        for (std::size_t i = 0; i < keep; ++i) {
            tmp[i] = s.resources.getByIdx(i);
        }
        s.resources.reset(n);
        for (std::size_t i = 0; i < s.resources.size(); ++i) {
            if (!isFiniteNonNegative(s.resources.getByIdx(i))) {
                s.resources.setByIdx(i, 0.0);
            }
            if (!std::isfinite(s.resources.rateAt(i))) {
                s.resources.setRate(i, 0.0);
            }
        }
    }
    // Units
    {
        std::vector<UnitStack> keep;
        keep.reserve(s.units.size());
        std::unordered_set<std::string> seen;
        for (auto& u : s.units) {
            if (u.count <= 0 || !C.findUnit(u.id)) {
                continue;
            }
            if (!seen.insert(u.id).second) {
                continue;
            }
            keep.push_back(std::move(u));
        }
        s.units = std::move(keep);
    }
    // Buildings
    {
        std::vector<UnitStack> keep;
        keep.reserve(s.buildings.size());
        std::unordered_set<std::string> seen;
        for (auto& b : s.buildings) {
            if (b.count <= 0) {
                continue;
            }
            if (!C.findBuilding(b.id)) {
                continue;
            }
            if (!seen.insert(b.id).second) {
                continue;
            }
            keep.push_back(std::move(b));
        }
        s.buildings = std::move(keep);
    }
    // Unlocked
    {
        for (const auto& u : C.allUnits()) {
            if (u.unlockedByDefault) {
                s.unlock(u.id);
            }
        }
        for (const auto& u : C.allUpgrades()) {
            if (u.unlockedByDefault) {
                s.unlock(u.id);
            }
        }
        for (const auto& b : C.allBuildings()) {
            if (b.unlockedByDefault) {
                s.unlock(b.id);
            }
        }
        std::unordered_set<std::string> cleaned;
        for (const auto& id : s.unlocked) {
            if (C.findUnit(id) || C.findUpgrade(id) || C.findBuilding(id)) {
                cleaned.insert(id);
            }
        }
        s.unlocked = std::move(cleaned);
    }
    // Upgrades
    {
        std::vector<UpgradeState> keep;
        keep.reserve(s.upgrades.size());
        std::unordered_set<std::string> seen;
        for (auto& up : s.upgrades) {
            const UpgradeDef* def = C.findUpgrade(up.id);
            if (!def) {
                continue;
            }
            if (!seen.insert(up.id).second) {
                continue;
            }
            if (up.level <= 0) {
                continue;
            }
            if (up.level > def->maxLevel) {
                up.level = def->maxLevel;
            }
            keep.push_back(std::move(up));
        }
        s.upgrades = std::move(keep);
    }
    // Era
    if (s.era != Era::CHILDHOOD && s.era != Era::ENTREPRENEUR) {
        s.era = Era::CHILDHOOD;
    }

    // Multipliers
    if (!isFiniteNonNegative(s.clickPower)) {
        s.clickPower = 1.0;
    }
    if (!isFiniteNonNegative(s.qualityMult)) {
        s.qualityMult = 1.0;
    }
    if (!isFiniteNonNegative(s.speedMult)) {
        s.speedMult = 1.0;
    }

    // Boss
    const int bossCount = C.bossCount(s.era);
    if (bossCount <= 0) {
        s.bossIndex = 0;
        s.currentBoss = Boss{};
        s.currentBoss.hp = 0.0;
        return;
    }
    if (s.bossIndex < 0) {
        s.bossIndex = 0;
    }
    if (s.bossIndex >= bossCount) {
        s.bossIndex = bossCount;
        s.currentBoss.hp = 0.0;
        return;
    }

    const BossDef* def = C.bossAt(s.era, s.bossIndex);
    if (!def) {
        s.currentBoss.hp = 0.0;
        return;
    }
    s.currentBoss.id = def->id;
    s.currentBoss.name = def->name;
    s.currentBoss.maxHp = def->maxHp;
    s.currentBoss.dps = def->dps;
    if (!std::isfinite(s.currentBoss.hp) || s.currentBoss.hp < 0.0) {
        s.currentBoss.hp = def->maxHp;
    }
    if (s.currentBoss.hp > def->maxHp) {
        s.currentBoss.hp = def->maxHp;
    }
}

inline int readInt(const json& o, const char* key, int fallback) {
    if (!o.contains(key)) {
        return fallback;
    }
    const auto& value = o[key];
    if (value.is_number_integer()) {
        return value.get<int>();
    }
    if (value.is_number_unsigned()) {
        return static_cast<int>(value.get<unsigned>());
    }
    return fallback;
}

inline std::int64_t readI64(const json& o, const char* key,
                            std::int64_t fallback) {
    if (!o.contains(key)) {
        return fallback;
    }
    const auto& value = o[key];
    if (value.is_number_integer()) {
        return value.get<std::int64_t>();
    }
    if (value.is_number_unsigned()) {
        return static_cast<std::int64_t>(value.get<std::uint64_t>());
    }
    return fallback;
}

inline double readDouble(const json& o, const char* key, double fallback) {
    if (!o.contains(key)) {
        return fallback;
    }
    const auto& value = o[key];
    if (value.is_number()) {
        return value.get<double>();
    }
    return fallback;
}

inline std::uint64_t readU64(const json& o, const char* key,
                             std::uint64_t fallback) {
    if (!o.contains(key)) {
        return fallback;
    }
    const auto& value = o[key];
    if (value.is_number_unsigned()) {
        return value.get<std::uint64_t>();
    }
    if (value.is_number_integer()) {
        const auto s = value.get<std::int64_t>();
        return s < 0 ? fallback : static_cast<std::uint64_t>(s);
    }
    return fallback;
}

inline std::string readString(const json& o, const char* key) {
    if (!o.contains(key)) {
        return {};
    }
    const auto& value = o[key];
    if (value.is_string()) {
        return value.get<std::string>();
    }
    return {};
}
}  // namespace
bool SaveSystem::save(GameState& state, const std::string& path) const {
    state.lastSaveTimestamp = nowSeconds();

    const auto& C = content::Content::instance();

    json j;
    j["version"] = kCurrentSaveVersion;
    j["era"] = static_cast<int>(state.era);

    json res = json::object();
    const auto& defs = C.allResources();
    const std::size_t rn = std::min(defs.size(), state.resources.size());
    for (std::size_t i = 0; i < rn; ++i) {
        res[defs[i].id] = state.resources.getByIdx(i);
    }
    j["resources"] = std::move(res);

    json units = json::array();
    for (const auto& u : state.units) {
        if (u.count <= 0) {
            continue;
        }
        units.push_back({{"id", u.id}, {"count", u.count}});
    }
    j["units"] = std::move(units);

    json builds = json::array();
    for (const auto& b : state.buildings) {
        if (b.count <= 0) {
            continue;
        }
        builds.push_back({{"id", b.id}, {"count", b.count}});
    }
    j["buildings"] = std::move(builds);

    j["unlocked"] = json::array();
    for (const auto& id : state.unlocked) {
        j["unlocked"].push_back(id);
    }

    json ups = json::array();
    for (const auto& u : state.upgrades) {
        if (u.level <= 0) {
            continue;
        }
        ups.push_back({{"id", u.id}, {"level", u.level}});
    }
    j["upgrades"] = std::move(ups);

    j["bossIndex"] = state.bossIndex;
    j["currentBossHp"] = state.currentBoss.hp;
    j["clickPower"] = state.clickPower;
    j["qualityMult"] = state.qualityMult;
    j["speedMult"] = state.speedMult;
    j["playTimeSeconds"] = state.playTimeSeconds;
    j["rngSeed"] = state.rngSeed;
    j["lastSaveTimestamp"] = state.lastSaveTimestamp;

    const std::string tmpPath = path + ".tmp";
    const std::string bakPath = path + ".bak";

    // write to temp file
    {
        std::ofstream out(tmpPath, std::ios::binary | std::ios::trunc);
        if (!out) {
            return false;
        }
        out << j.dump(2);
        out.flush();
        if (!out.good()) {
            return false;
        }
    }

    std::error_code ec;

    // best-effort backup of previous save.
    if (std::filesystem::exists(path, ec) && !ec) {
        std::filesystem::copy_file(
            path, bakPath, std::filesystem::copy_options::overwrite_existing,
            ec);
        ec.clear();
        // ignore errors - the primary save is what matters
    }

    // atomic replace
    std::filesystem::rename(tmpPath, path, ec);
    if (ec) {
        std::error_code rmEc;
        std::filesystem::remove(tmpPath, rmEc);
        return false;
    }
    return true;
}

bool SaveSystem::load(GameState& state, const std::string& path) const {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return false;
    }

    json j;
    try {
        in >> j;
    } catch (...) {
        return false;
    }
    if (!j.is_object()) {
        return false;
    }
    try {
        const std::uint32_t ver =
            static_cast<std::uint32_t>(readI64(j, "version", 1));
        if (ver > kCurrentSaveVersion) {
            return false;
        }

        const auto& C = content::Content::instance();
        const auto& defs = C.allResources();

        GameState loaded;
        loaded.version = ver;
        loaded.era = static_cast<Era>(readInt(j, "era", 1));

        loaded.resources.reset(defs.size());
        if (j.contains("resources") && j["resources"].is_object()) {
            const auto& res = j["resources"];
            for (std::size_t i = 0; i < defs.size(); ++i) {
                loaded.resources.setByIdx(
                    i, readDouble(res, defs[i].id.c_str(), 0.0));
            }
        }

        if (j.contains("units") && j["units"].is_array()) {
            for (const auto& u : j["units"]) {
                if (!u.is_object()) {
                    continue;
                }
                const std::string id = readString(u, "id");
                const int count = readInt(u, "count", 0);
                if (id.empty() || count <= 0) {
                    continue;
                }
                loaded.units.push_back(UnitStack{id, count});
            }
        }
        if (j.contains("buildings") && j["buildings"].is_array()) {
            for (const auto& b : j["buildings"]) {
                if (!b.is_object()) {
                    continue;
                }
                const std::string id = readString(b, "id");
                const int count = readInt(b, "count", 0);
                if (id.empty() || count <= 0) {
                    continue;
                }
                loaded.buildings.push_back(UnitStack{id, count});
            }
        }
        if (j.contains("unlocked") && j["unlocked"].is_array()) {
            for (const auto& id : j["unlocked"]) {
                if (id.is_string()) {
                    loaded.unlocked.insert(id.get<std::string>());
                }
            }
        }
        if (j.contains("upgrades") && j["upgrades"].is_array()) {
            for (const auto& u : j["upgrades"]) {
                if (!u.is_object()) {
                    continue;
                }
                const std::string id = readString(u, "id");
                const int level = readInt(u, "level", 0);
                if (id.empty() || level <= 0) {
                    continue;
                }
                loaded.upgrades.push_back(UpgradeState{id, level});
            }
        }
        loaded.bossIndex = readInt(j, "bossIndex", 0);
        loaded.clickPower = readDouble(j, "clickPower", 1.0);
        loaded.qualityMult = readDouble(j, "qualityMult", 1.0);
        loaded.speedMult = readDouble(j, "speedMult", 1.0);
        loaded.playTimeSeconds = readI64(j, "playTimeSeconds", 0);
        loaded.rngSeed = readI64(j, "rngSeed", 0xDEADBEEFCAFEF00DULL);
        loaded.lastSaveTimestamp = readI64(j, "lastSaveTimestamp", 0);

        if (const BossDef* b = C.bossAt(loaded.era, loaded.bossIndex)) {
            loaded.currentBoss =
                Boss{b->id, b->name, b->maxHp, b->maxHp, b->dps};
            loaded.currentBoss.hp = readDouble(j, "currentBossHp", b->maxHp);
        }

        sanitize(loaded);

        state = std::move(loaded);
        return true;
    } catch (...) {
        return false;
    }
}

bool SaveSystem::needAutosave(const GameState& state, std::int64_t now) {
    if (state.lastSaveTimestamp <= 0) {
        return false;
    }
    if (now == 0) {
        now = nowSeconds();
    }
    const std::int64_t interval =
        content::Content::instance().balance().autosaveIntervalSeconds;
    if (interval <= 0) {
        return false;
    }
    return (now - state.lastSaveTimestamp) >= interval;
}

// offline
std::int64_t SaveSystem::computeOfflineSeconds(const GameState& state,
                                               std::int64_t now) {
    if (state.lastSaveTimestamp <= 0) {
        return 0;
    }
    if (now == 0) {
        now = nowSeconds();
    }
    const std::int64_t diff = now - state.lastSaveTimestamp;
    return diff > 0 ? diff : 0;
}

double SaveSystem::applyOffline(GameState& state, GameLogic& logic,
                                std::int64_t secondsAway) {
    const auto& bal = content::Content::instance().balance();

    if (secondsAway < bal.offlineMinSeconds) {
        return 0.0;
    }
    if (secondsAway > bal.offlineMaxSeconds) {
        secondsAway = bal.offlineMaxSeconds;
    }
    const double subTickMax =
        bal.offlineSubTickMaxSeconds > 0.0 ? bal.offlineSubTickMaxSeconds : 1.0;
    constexpr std::int64_t kMaxSubTicks = 1'000'000;

    std::int64_t remaining = secondsAway;
    double effectiveSeconds = 0.0;
    std::int64_t subTicks = 0;

    for (const auto& seg : bal.offlineSegments) {
        if (remaining <= 0 || subTicks >= kMaxSubTicks) {
            break;
        }
        if (seg.seconds <= 0 || seg.speed <= 0.0) {
            continue;
        }

        const std::int64_t step = std::min(remaining, seg.seconds);
        remaining -= step;

        double sim = static_cast<double>(step) * seg.speed;
        effectiveSeconds += sim;

        while (sim > 0.0 && subTicks < kMaxSubTicks) {
            const double sub = std::min(sim, subTickMax);
            logic.tick(state, TickContext{sub, true, seg.speed});
            sim -= sub;
            ++subTicks;
        }
    }
    return effectiveSeconds;
}
std::int64_t game::SaveSystem::nowSeconds() {
    using namespace std::chrono;
    return duration_cast<seconds>(system_clock::now().time_since_epoch())
        .count();
}
}  // namespace game