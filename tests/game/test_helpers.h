#pragma once
#include <empire/game/content/Content.h>
#include <empire/game/state/GameState.h>

// Test-side helpers to bridge to current API (no more ResourceType enum).
namespace test {

inline game::ResourceId plastic() {
    return game::content::Content::instance().resourceId("plastic");
}
inline game::ResourceId paper() {
    return game::content::Content::instance().resourceId("paper");
}
inline game::ResourceId glue() {
    return game::content::Content::instance().resourceId("glue");
}

inline void resetPoolToContent(game::ResourcePool& p) {
    p.reset(game::content::Content::instance().allResources().size());
}

}  // namespace test