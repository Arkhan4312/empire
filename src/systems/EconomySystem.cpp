#include "systems/EconomySystem.h"

#include <cmath>
#include <utility>
#include <vector>

#include "content/Content.h"

namespace game {
void EconomySystem::tick(GameState& state, double dt) {
    const auto& C = content::Content::instance();
    const std::size_t rn = state.resources.size();

    for (std::size_t i = 0; i < rn; ++i) {
        state.resources.setRate(i, 0.0);
    }
    for (const auto& stack : state.buildings) {
        const BuildingDef* def = C.findBuilding(stack.id);
        if (!def) {
            continue;
        }
        for (const auto& p : def->production) {
            if (p.id == kInvalidResource) {
                continue;
            }
            state.resources.addRate(p.id, p.amount * stack.count);
        }
    }

    for (std::size_t i = 0; i < rn; ++i) {
        state.resources.addByIdx(i, state.resources.rateAt(i) * dt);
    }
}

bool EconomySystem::build(GameState& state, const std::string& buildingId,
                          int count) {
    if (count <= 0) {
        return false;
    }
    if (!state.isUnlocked(buildingId)) {
        return false;
    }
    const BuildingDef* def =
        content::Content::instance().findBuilding(buildingId);
    if (!def) {
        return false;
    }
    const int owned = state.buildingCount(buildingId);

    std::vector<std::pair<ResourceId, double>> totals;
    totals.reserve(def->costs.size());
    for (const auto& c : def->costs) {
        if (c.id == kInvalidResource) {
            continue;
        }
        double sum = 0.0;
        for (int i = 0; i < count; ++i) {
            sum += c.amount * std::pow(def->costMultiplier, owned + i);
        }
        totals.emplace_back(c.id, sum);
    }
    for (auto& [rid, amt] : totals) {
        if (state.resources.get(rid) < amt) {
            return false;
        }
    }
    for (auto& [rid, amt] : totals) {
        state.resources.spend(rid, amt);
    }

    if (UnitStack* stack = state.findBuilding(buildingId)) {
        stack->count += count;
    } else {
        state.buildings.push_back(UnitStack{buildingId, count});
    }
    return true;
}

double EconomySystem::nextCost(const GameState& state,
                               const std::string& buildingId,
                               ResourceId resourceId) const {
    const BuildingDef* def =
        content::Content::instance().findBuilding(buildingId);
    if (!def) {
        return 0.0;
    }
    const int owned = state.buildingCount(buildingId);
    for (const auto& c : def->costs) {
        if (c.id == resourceId) {
            return c.amount * std::pow(def->costMultiplier, owned);
        }
    }
    return 0.0;
}

double EconomySystem::totalRate(const GameState& state,
                                std::size_t resourceIdx) const {
    return state.resources.rateAt(resourceIdx);
}
}  // namespace game