#include "systems/ArmySystem.h"

#include "content/Content.h"

namespace game {
void ArmySystem::tick(GameState& state, double dt) {
    if (state.era != Era::CHILDHOOD) return;

    const double dps = computeArmyDps(state);
    if (dps > 0.0) {
        damageBoss(state, dps, dt);
    }
}

bool ArmySystem::craft(GameState& state, const std::string& unitId, int count) {
    if (count <= 0) {
        return false;
    }
    if (!state.isUnlocked(unitId)) {
        return false;
    }
    const UnitDef* def = content::findUnit(unitId);
    if (!def) {
        return false;
    }

    for (const auto& c : def->costs) {
        if (c.id == kInvalidResource) {
            continue;
        }
        if (state.resources.get(c.id) < c.amount * count) {
            return false;
        }
    }
    for (const auto& c : def->costs) {
        if (c.id == kInvalidResource) {
            continue;
        }
        state.resources.spend(c.id, c.amount * count);
    }

    if (UnitStack* stack = state.findUnit(unitId)) {
        stack->count += count;
    } else {
        state.units.push_back(UnitStack{unitId, count});
    }
    return true;
}

double ArmySystem::computeArmyDps(const GameState& state) const {
    double dps = 0.0;
    for (const auto& stack : state.units) {
        const UnitDef* def = content::findUnit(stack.id);
        if (!def) {
            continue;
        }
        dps +=
            def->damage * static_cast<double>(stack.count) * state.qualityMult;
    }
    return dps;
}

void ArmySystem::damageBoss(GameState& state, double dps, double dt) {
    const auto& C = content::Content::instance();
    double remaining = dps * dt;
    if (remaining <= 0.0) {
        return;
    }
    // loop
    while (remaining > 0.0) {
        if (state.bossIndex >= C.bossCount(state.era)) {
            state.currentBoss.hp = 0.0;
            return;
        }
        state.currentBoss.hp -= remaining;
        if (state.currentBoss.hp > 0.0) {
            return;
        }

        const double overflow = -state.currentBoss.hp;
        const BossDef* killed = C.bossAt(state.era, state.bossIndex);
        if (killed) {
            for (const auto& r : killed->reward) {
                if (r.id != kInvalidResource) {
                    state.resources.add(r.id, r.amount);
                }
            }
            for (const auto& id : killed->unlocks) {
                state.unlock(id);

                std::string display = id;
                if (const auto* u = C.findUnit(id)) {
                    display = u->name;
                } else if (const auto* up = C.findUpgrade(id)) {
                    display = up->name;
                } else if (const auto* b = C.findBuilding(id)) {
                    display = b->name;
                }
                state.events.push_back({GameEventKind::Unlock, id, display});
            }
            state.events.push_back(
                {GameEventKind::BossKilled, killed->id, killed->name});
        }

        ++state.bossIndex;

        const BossDef* next = C.bossAt(state.era, state.bossIndex);
        if (!next) {
            state.currentBoss.hp = 0.0;
            return;
        }
        state.currentBoss = Boss{next->id, next->name, next->maxHp,
                                 next->maxHp - overflow, next->dps};
        remaining = overflow;
    }
}
}  // namespace game