#pragma once
#include <cstdint>
#include <string>

#include "core/GameState.h"

namespace game {

class GameLogic;  // fwd

// Persistent storage + offline progression for GameState.

class SaveSystem {
public:
    static constexpr std::int64_t kAutoSaveInterval = 60;
    
    // save() is atomic: writes to "<path>.tmp" then renames over the original.
    // save() updates state.lastSaveTimestamp to the current wall clock, so
    // callers don't have to do it manually
    
    bool save(GameState& state, const std::string& path) const;
    // load() validates and sanitizes the state: values are clamped, unknown ids
    // are dropped, boss metadata is refreshed from Content.

    bool load(GameState& state, const std::string& path) const;

    // True if state should be autosaved now.
    // Return false if the state was never saved.
    static bool needAutosave(const GameState& state, std::int64_t now = 0);

    // compute offline
    static std::int64_t computeOfflineSeconds(const GameState& state,
                                              std::int64_t now = 0);
    // Offline fprogression uses small subticks so systems that assume a small
    // dt behave correctly across hours/days.
    static double applyOffline(GameState& state, GameLogic& logic,
                               std::int64_t secondsAway);

    // helper
    static std::int64_t nowSeconds();
};
}  // namespace game