#pragma once
#include <cstdint>
#include <string>

#include "core/GameState.h"

namespace game {

class GameLogic;  // fwd

class SaveSystem {
public:
    // simple s/l system
    bool save(const GameState& state, const std::string& path) const;
    bool load(GameState& state, const std::string& path) const;

    // compute offline
    static std::int64_t computeOfflineSeconds(const GameState& state,
                                              std::int64_t now = 0);
    static double applyOffline(GameState& state, GameLogic& logic,
                               std::int64_t secondsAway);
};
}  // namespace game