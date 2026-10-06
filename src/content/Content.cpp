#include "Content.h"

#include <algorithm>
#include <fstream>
#include <mutex>
#include <nlohmann/json.hpp>
#include <sstream>
#include <unordered_set>

#include "Content.h"
#include "content/Content.h"
#include "data/ResourceDef.h"

namespace game::content {

using nlohmann::json;

const Balance& Balance::defaults() {
    static const Balance b = [] {
        Balance x;
        x.clickBasePower = 1.0;
        x.autosaveIntervalSeconds = 60;
        x.offlineMinSeconds = 60;
        x.offlineMaxSeconds = 7LL * 24 * 3600;
        x.offlineSubTickMaxSeconds = 1.0;
        x.offlineSegments = {
            {3600, 1.00},
            {18000, 0.75},
            {64800, 0.50},
            {518400, 0.25},
        };
        return x;
    }();
    return b;
}
// effect registery
namespace {
std::unordered_map<std::string, EffectFn>& effectRegistry() {
    static std::unordered_map<std::string, EffectFn> r;
    return r;
}
std::once_flag g_effectsOnce;

void registerBuiltinEffects() {
    registerEffect("quality_mult_mul",
                   [](GameState& s, double v) { s.qualityMult *= v; });
    registerEffect("quality_mult_add",
                   [](GameState& s, double v) { s.qualityMult += v; });
    registerEffect("click_power_mul",
                   [](GameState& s, double v) { s.clickPower *= v; });
    registerEffect("click_power_add",
                   [](GameState& s, double v) { s.clickPower += v; });
    registerEffect("speed_mult_mul",
                   [](GameState& s, double v) { s.speedMult *= v; });
}
}  // namespace

void registerEffect(std::string name, EffectFn fn) {
    effectRegistry()[std::move(name)] = std::move(fn);
}

bool hasEffect(std::string_view name) {
    std::call_once(g_effectsOnce, registerBuiltinEffects);
    return effectRegistry().count(std::string(name)) > 0;
}

// parsing helpers
namespace {
bool parseCosts(const json& obj,
                std::unordered_map<std::string, ResourceId>& resIdx,
                std::vector<ResourceAmount>& out, std::string& err) {
    if (!obj.is_object()) {
        err = "'cost' must be an object";
        return false;
    }
    out.clear();
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        if (!it.value().is_number()) {
            err = "cost." + it.key() + " must be a number";
            return false;
        }
        const double v = it.value().get<double>();
        if (!std::isfinite(v) || v < 0.0) {
            err = "cost." + it.key() + " must be a finite number >= 0";
            return false;
        }
        auto ridIt = resIdx.find(it.key());
        if (ridIt == resIdx.end()) {
            err = "unknown resource '" + it.key() + "'";
            return false;
        }
        out.push_back({ridIt->second, v});
    }
    return true;
}

bool parseEra(const std::string& s, Era& out) {
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

bool loadJsonFile(const std::string& path, json& out, std::string& err) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        err = "cannot open " + path;
        return false;
    }
    try {
        f >> out;
    } catch (const std::exception& e) {
        err = std::string("JSON parse: ") + e.what();
        return false;
    }
    return true;
}
}  // namespace

Content& Content::instance() {
    static Content inst;
    return inst;
}

Content::Content() {
    std::call_once(g_effectsOnce, registerBuiltinEffects);
    installFallback();
}

// load
bool game::content::Content::loadFromDirectory(const std::string& dir,
                                               std::string* errOut) {
    std::string err;
    auto fail = [&](const std::string& e) {
        if (errOut) {
            *errOut = e;
        }
        return false;
    };
    // balance.json
    Balance newBalance = Balance::defaults();
    {
        const std::string path = dir + "/balance.json";
        json j;
        std::ifstream f(path, std::ios::binary);
        if (f) {
            try {
                f >> j;
            } catch (const std::exception& e) {
                return fail(std::string("balance.json: ") + e.what());
            }

            if (auto it = j.find("click"); it != j.end() && it->is_object()) {
                newBalance.clickBasePower =
                    it->value("basePower", newBalance.clickBasePower);
            }
            if (auto it = j.find("autosave");
                it != j.end() && it->is_object()) {
                newBalance.autosaveIntervalSeconds = it->value(
                    "intervalSeconds", newBalance.autosaveIntervalSeconds);
            }
            if (auto it = j.find("offline"); it != j.end() && it->is_object()) {
                const auto& o = *it;
                newBalance.offlineMinSeconds =
                    o.value("minSeconds", newBalance.offlineMinSeconds);
                newBalance.offlineMaxSeconds =
                    o.value("maxSeconds", newBalance.offlineMaxSeconds);
                newBalance.offlineSubTickMaxSeconds = o.value(
                    "subTickMaxSeconds", newBalance.offlineSubTickMaxSeconds);
                if (o.contains("segments") && o["segments"].is_array()) {
                    newBalance.offlineSegments.clear();
                    for (const auto& s : o["segments"]) {
                        if (!s.is_object()) {
                            continue;
                        }
                        OfflineSegment seg;
                        seg.seconds = s.value("seconds", (std::int64_t)0);
                        seg.speed = s.value("speed", 1.0);
                        if (seg.seconds > 0 && seg.speed > 0.0) {
                            newBalance.offlineSegments.push_back(seg);
                        }
                    }
                    if (newBalance.offlineSegments.empty()) {
                        newBalance.offlineSegments =
                            Balance::defaults().offlineSegments;
                    }
                }
            }
        }
    }
    // resources
    std::vector<ResourceDef> newResources;
    {
        json j;
        if (!loadJsonFile(dir + "/resources.json", j, err)) {
            return fail(err);
        }
        if (!j.contains("resources") || !j["resources"].is_array()) {
            return fail("resources.json: missing 'resources' array");
        }
        if (j["resources"].size() > kInvalidResource) {
            return fail("resources.json: too many resources (max 65535)");
        }

        std::unordered_set<std::string> seen;
        for (const auto& r : j["resources"]) {
            if (!r.is_object()) {
                return fail("resources.json: resource is not an object");
            }
            ResourceDef d;
            d.id = r.value("id", "");
            d.name = r.value("name", d.id);
            if (d.id.empty()) {
                return fail("resources.json: resource without id");
            }
            if (!seen.insert(d.id).second) {
                return fail("resources.json: duplicate id '" + d.id + "'");
            }
            if (!std::isalnum(static_cast<unsigned char>(d.id[0])) &&
                d.id[0] != '_') {
                return fail("resources.json: id must start with [a-zA-Z_]");
            }
            d.clickYield = r.value("clickYield", 0.0);
            d.baseRate = r.value("baseRate", 0.0);
            if (d.clickYield < 0.0 || d.baseRate < 0.0) {
                return fail("resources.json[" + d.id +
                            "]: negative yield/rate");
            }
            newResources.push_back(std::move(d));
        }
    }
    std::unordered_map<std::string, ResourceId> resIdx;
    resIdx.reserve(newResources.size());
    for (std::size_t i = 0; i < newResources.size(); ++i) {
        resIdx[newResources[i].id] = static_cast<ResourceId>(i);
    }
    // units
    std::vector<UnitDef> newUnits;
    {
        json j;
        if (!loadJsonFile(dir + "/units.json", j, err)) {
            return fail(err);
        }
        if (!j.contains("units") || !j["units"].is_array()) {
            return fail("units.json: missing 'units' array");
        }
        std::unordered_set<std::string> seen;
        for (const auto& u : j["units"]) {
            if (!u.is_object()) {
                return fail("units.json: unit is not an object");
            }
            UnitDef d;
            d.id = u.value("id", "");
            d.name = u.value("name", d.id);
            if (d.id.empty()) {
                return fail("units.json: unit without id");
            }
            if (!seen.insert(d.id).second) {
                return fail("units.json: duplicate id '" + d.id + "'");
            }
            if (u.contains("cost")) {
                std::string cerr;
                if (!parseCosts(u["cost"], resIdx, d.costs, cerr)) {
                    return fail("units.json[" + d.id + "]: " + cerr);
                }
            }
            if (u.contains("stats")) {
                if (!u["stats"].is_object()) {
                    return fail("uints.json[" + d.id +
                                "]: 'stats' must be an object");
                }
                const auto& s = u["stats"];
                d.damage = s.value("damage", d.damage);
                d.hp = s.value("hp", d.hp);
                d.speed = s.value("speed", d.speed);
                if (!std::isfinite(d.damage) || d.damage < 0.0 ||
                    !std::isfinite(d.hp) || d.hp <= 0.0 ||
                    !std::isfinite(d.speed) || d.speed < 0.0) {
                    return fail("units.json[" + d.id + "]: invalid stats");
                }
            }
            newUnits.push_back(std::move(d));
        }
    }

    // upgrades
    std::vector<UpgradeDef> newUpgrades;
    {
        json j;
        if (!loadJsonFile(dir + "/upgrades.json", j, err)) {
            return fail(err);
        }
        if (!j.contains("upgrades") || !j["upgrades"].is_array()) {
            return fail("upgrades.json: missing 'upgrades' array");
        }
        std::unordered_set<std::string> seen;
        for (const auto& u : j["upgrades"]) {
            if (!u.is_object()) {
                return fail("upgrades.json: upgrade is not an object");
            }
            UpgradeDef d;
            d.id = u.value("id", "");
            d.name = u.value("name", d.id);
            if (d.id.empty()) {
                return fail("upgrades.json: upgrade without id");
            }
            if (!seen.insert(d.id).second) {
                return fail("upgrades.json: duplicate id '" + d.id + "'");
            }
            if (u.contains("cost")) {
                std::string cerr;
                if (!parseCosts(u["cost"], resIdx, d.costs, cerr)) {
                    return fail("upgrades.json[" + d.id + "]: " + cerr);
                }
            }

            d.maxLevel = u.value("maxLevel", 1);
            if (d.maxLevel < 1) {
                return fail("upgrades.json[" + d.id + "]: maxLevel < 1");
            }

            if (u.contains("effects")) {
                if (!u["effects"].is_array()) {
                    return fail("upgrades.json[" + d.id +
                                "]: effects not an array");
                }
                for (const auto& e : u["effects"]) {
                    if (!e.is_object()) {
                        return fail("upgrades.json[" + d.id +
                                    "]: effect not an object");
                    }
                    const std::string name = e.value("name", "");
                    const double val = e.value("value", 0.0);
                    if (name.empty()) {
                        return fail("upgrades.json[" + d.id +
                                    "]: effect without name");
                    }
                    if (!std::isfinite(val)) {
                        return fail("upgrades.json[" + d.id + "]: effect '" +
                                    name + "' non-finite value");
                    }
                    if (!hasEffect(name)) {
                        return fail("upgrades.json[" + d.id +
                                    "]: unknown effect '" + name + "'");
                    }
                    d.effects.push_back({effectRegistry().at(name), val});
                }
            }
            newUpgrades.push_back(std::move(d));
        }
    }

    // bosses
    std::vector<BossDef> newBosses;
    {
        json j;
        if (!loadJsonFile(dir + "/bosses.json", j, err)) {
            return fail(err);
        }
        if (!j.contains("bosses") || !j["bosses"].is_array()) {
            return fail("bosses.json: missing 'bosses' array");
        }

        std::unordered_set<std::string> seen;
        for (const auto& b : j["bosses"]) {
            if (!b.is_object()) {
                return fail("bosses.json: boss is not an object");
            }
            BossDef d;
            d.id = b.value("id", "");
            d.name = b.value("name", d.id);
            if (d.id.empty()) {
                return fail("bosses.json: boss without id");
            }
            if (!seen.insert(d.id).second) {
                return fail("bosses.json: duplicate id '" + d.id + "'");
            }
            Era era;
            const std::string eraStr = b.value("era", "CHILDHOOD");
            if (!parseEra(eraStr, era)) {
                return fail("bosses.json[" + d.id + "]: unknown era '" +
                            eraStr + "'");
            }
            d.era = era;

            d.maxHp = b.value("maxHp", 100.0);
            d.dps = b.value("dps", 1.0);
            if (d.maxHp <= 0.0) {
                return fail("bosses.json[" + d.id + "]: maxHp <= 0");
            }
            newBosses.push_back(std::move(d));
        }
    }
    m_resources = std::move(newResources);
    m_units = std::move(newUnits);
    m_upgrades = std::move(newUpgrades);
    m_bosses = std::move(newBosses);

    m_balance = std::move(newBalance);
    m_lastDir = dir;
    m_loadedFromDisk = true;
    rebuildIndices();
    rebuildEraBuckets();

    m_balance.childhoodBossCount = bossCount(Era::CHILDHOOD);
    return true;
}

bool game::content::Content::reload(std::string* err) {
    if (m_lastDir.empty()) {
        if (err) {
            *err = "no directory loaded yet";
            return false;
        }
    }
    return loadFromDirectory(m_lastDir, err);
}

const UnitDef* game::content::Content::findUnit(std::string_view id) const {
    auto it = m_unitIdx.find(std::string(id));
    return it == m_unitIdx.end() ? nullptr : &m_units[it->second];
}
const UpgradeDef* game::content::Content::findUpgrade(
    std::string_view id) const {
    auto it = m_upgradeIdx.find(std::string(id));
    return it == m_upgradeIdx.end() ? nullptr : &m_upgrades[it->second];
}

const BossDef* Content::findBoss(std::string_view id) const {
    auto it = m_bossIdx.find(std::string(id));
    return it == m_bossIdx.end() ? nullptr : &m_bosses[it->second];
}

const ResourceDef* Content::resource(ResourceId id) const {
    return id < m_resources.size() ? &m_resources[id] : nullptr;
}

ResourceId Content::resourceId(std::string_view id) const {
    auto it = m_resourceIdx.find(std::string(id));
    return it == m_resourceIdx.end() ? kInvalidResource : it->second;
}

const BossDef* game::content::Content::bossAt(Era era, int index) const {
    auto it = m_bossesByEra.find(static_cast<int>(era));
    if (it == m_bossesByEra.end()) {
        return nullptr;
    }
    if (index < 0 || static_cast<std::size_t>(index) >= it->second.size()) {
        return nullptr;
    }
    return &m_bosses[it->second[static_cast<std::size_t>(index)]];
}

int game::content::Content::bossCount(Era era) const {
    auto it = m_bossesByEra.find(static_cast<int>(era));
    return it == m_bossesByEra.end() ? 0 : static_cast<int>(it->second.size());
}

void game::content::Content::installFallback() {
    m_resources = {
        {"plastic", "Пластилин", 1.0, 0.0},
        {"paper", "Бумага", 0.0, 0.0},
        {"glue", "Клей", 0.0, 0.0},
    };
    m_units.clear();
    {
        UnitDef u;
        u.id = "soldier_crooked";
        u.name = "Солдатик с кривыми руками";
        u.costs = {{0, 5.0}};
        u.damage = 1.0;
        u.hp = 5.0;
        u.speed = 1.0;
        m_units.push_back(std::move(u));

        u = {};
        u.id = "tank_matchbox";
        u.name = "Танк из спичечного коробка";
        u.costs = {{0, 25.0}};
        u.damage = 3.0;
        u.hp = 20.0;
        u.speed = 0.5;
        m_units.push_back(std::move(u));

        u = {};
        u.id = "plane_paper";
        u.name = "Бумажный самолётик";
        u.costs = {{1, 10.0}};
        u.damage = 2.0;
        u.hp = 3.0;
        u.speed = 3.0;
        m_units.push_back(std::move(u));
    }
    {
        UpgradeDef up;
        up.id = "up_plastic_1";
        up.name = "Новый набор пластилина";
        up.costs = {{0, 100.0}};
        up.maxLevel = 5;
        if (auto it = effectRegistry().find("quality_mult_mul");
            it != effectRegistry().end()) {
            up.effects.push_back({it->second, 1.25});
        }

        up = {};
        up.id = "up_tools_1";
        up.name = "Папины инструменты";
        up.costs = {{0, 500.0}};
        up.maxLevel = 3;
        if (auto it = effectRegistry().find("quality_mult_mul");
            it != effectRegistry().end()) {
            up.effects.push_back({it->second, 1.10});
        }
        m_upgrades.push_back(std::move(up));
    }
    m_bosses = {
        {"boss_kolya", "Колька из 3-го подъезда", Era::CHILDHOOD, 50, 2},
        {"boss_serega", "Старшеклассник Серёга", Era::CHILDHOOD, 150, 5},
        {"boss_vitka", "Местный задира Витька", Era::CHILDHOOD, 500, 15},
    };
    m_balance = Balance::defaults();
    m_balance.childhoodBossCount = 3;
    rebuildIndices();
    rebuildEraBuckets();
}

void game::content::Content::rebuildIndices() {
    m_unitIdx.clear();
    m_upgradeIdx.clear();
    m_bossIdx.clear();
    m_resourceIdx.clear();
    for (std::size_t i = 0; i < m_resources.size(); ++i) {
        m_resourceIdx[m_resources[i].id] = static_cast<ResourceId>(i);
    }
    for (std::size_t i = 0; i < m_units.size(); ++i) {
        m_unitIdx[m_units[i].id] = i;
    }
    for (std::size_t i = 0; i < m_upgrades.size(); ++i) {
        m_upgradeIdx[m_upgrades[i].id] = i;
    }
    for (std::size_t i = 0; i < m_bosses.size(); ++i) {
        m_bossIdx[m_bosses[i].id] = i;
    }
}
void game::content::Content::rebuildEraBuckets() {
    m_bossesByEra.clear();
    for (std::size_t i = 0; i < m_bosses.size(); ++i) {
        m_bossesByEra[static_cast<int>(m_bosses[i].era)].push_back(i);
    }
}

void initNewGame(GameState& state) {
    state = GameState{};
    const auto& content = Content::instance();
    state.era = Era::CHILDHOOD;
    state.bossIndex = 0;
    state.clickPower = Content::instance().balance().clickBasePower;
    state.resources.reset(content.allResources().size());

    if (const BossDef* b = Content::instance().bossAt(state.era, 0)) {
        state.currentBoss = Boss{b->id, b->name, b->maxHp, b->maxHp, b->dps};
    }
}
}  // namespace game::content