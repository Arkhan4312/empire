#include "scenes/SceneManager.h"

#include <utility>

#include "scenes/AppContext.h"
#include "ui/UIContext.h"

namespace game {
SceneManager::~SceneManager() = default;

void SceneManager::replace(ScenePtr scene, AppContext& ctx) {
    if (m_current) {
        m_current->onExit(ctx);
    }
    m_current = std::move(scene);

    ctx.ui.hovered = nullptr;
    ctx.ui.active = nullptr;
    ctx.ui.focused = nullptr;

    if (m_current) {
        m_current->onEnter(ctx);
    }
}

void SceneManager::requestReplace(ScenePtr scene) {
    m_pending = std::move(scene);
}

void SceneManager::applyPending(AppContext& ctx) {
    if (!m_pending) {
        return;
    }
    replace(std::move(m_pending), ctx);
    m_pending.reset();
}

void SceneManager::clear(AppContext& ctx) {
    if (m_current) {
        m_current->onExit(ctx);
        m_current.reset();
    }
    ctx.ui.hovered = nullptr;
    ctx.ui.active = nullptr;
    ctx.ui.focused = nullptr;
}
}  // namespace game