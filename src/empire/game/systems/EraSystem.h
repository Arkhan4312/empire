#pragma once
#include <empire/game/state/GameState.h>

namespace game {

class EraSystem {
public:
    void checkTransition(GameState& state);
    bool advanceEra(GameState& state);
};
}  // namespace game