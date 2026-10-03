#pragma once
#include "scenes/Scene.h"

namespace game {

struct AppContext;

class SceneManager {
public:
    SceneManager() = default;
    ~SceneManager();

    SceneManager(const SceneManager&) = delete;
    SceneManager& operator=(const SceneManager&) = delete;
    // instant replacement
    void replace(ScenePtr scene, AppContext& ctx);
    // deferred replacement
    void requestReplace(ScenePtr scene);
    void applyPending(AppContext& ctx);

    // clear scene without replaace
    void clear(AppContext& ctx);

    Scene* current() const noexcept {
        return m_current.get();
    }

private:
    ScenePtr m_current;
    ScenePtr m_pending;
};
}  // namespace game