#include "scenes/GameScene.h"

#include <cstdio>
#include <memory>

#include "content/Content.h"
#include "core/GameLogic.h"
#include "core/GameState.h"
#include "core/Input.h"
#include "core/InputSystem.h"
#include "render/Renderer.h"
#include "render/Window.h"
#include "scenes/AppContext.h"
#include "scenes/MainMenuScene.h"
#include "scenes/SceneManager.h"
#include "ui/UIContext.h"

namespace game {

void GameScene::onEnter(AppContext& ctx) {
    buildUi(ctx);
    refreshLabels(ctx);
}

void GameScene::onExit(AppContext& /*ctx*/) {
    // game state living without scene.
}

void GameScene::update(AppContext& ctx, double dt) {
    ctx.logic.tick(ctx.state, TickContext{dt});

    const InputState& in = ctx.input.state();
    if (in.isKeyPressed(keys::Space)) {
        ctx.logic.clickMain(ctx.state);
    }
    if (in.isKeyPressed(keys::E)) {
        ctx.logic.buyUpgrade(ctx.state, "up_plastic_1");
    }
    if (in.isKeyPressed(49)) {
        ctx.logic.craftUnit(ctx.state, "soldier_crooked");
    }
    if (in.isKeyPressed(50)) {
        ctx.logic.craftUnit(ctx.state, "tank_matchbox");
    }
    if (in.isKeyPressed(51)) {
        ctx.logic.craftUnit(ctx.state, "plane_paper");
    }
    if (in.isKeyPressed(keys::Escape)) {
        ctx.scenes.requestReplace(std::make_unique<MainMenuScene>());
        return;
    }

    refreshLabels(ctx);

    int w = 0;
    int h = 0;
    ctx.window.framebufferSize(w, h);
    const float margin = 20.0f;
    {
        const glm::vec2 avail{320.0f, static_cast<float>(h) - margin * 2.0f};
        glm::vec2 measured = m_hud.measure(ctx.ui, avail);

        measured.x = 320.0f;
        measured.y = std::min(measured.y, avail.y);

        m_hud.arrange(ctx.ui, {margin, margin}, measured);
        m_hud.update(ctx.ui);
    }
    {
        const glm::vec2 avail{320.0f, static_cast<float>(h) - margin * 2.0f};
        const glm::vec2 measured = m_actions.measure(ctx.ui, avail);
        const glm::vec2 pos{static_cast<float>(w) - measured.x - margin,
                            static_cast<float>(h) - measured.y - margin};
        m_actions.arrange(ctx.ui, pos, measured);
        m_actions.update(ctx.ui);
    }
}

void GameScene::render(AppContext& ctx) {
    m_hud.render(ctx.ui);
    m_actions.render(ctx.ui);
}
void GameScene::buildUi(AppContext& ctx) {
    m_actions.clear();
    m_actions.padding = 16.0f;
    m_actions.drawBackground = true;
    m_actions.background = ctx.ui.theme.panelBg;
    m_actions.borderColor = ctx.ui.theme.panelBorder;
    m_actions.borderWidth = 1.0f;
    m_actions.layout = std::make_unique<ui::BoxLayout>(
        ui::MainAxis::Vertical, ui::CrossAlign::Stretch, 8.0f);

    m_click = m_actions.add<ui::Button>("Click (space)");
    m_click->OnClick = [&ctx] { ctx.logic.clickMain(ctx.state); };

    m_upgrade = m_actions.add<ui::Button>("Buy upgrade (e)");
    m_upgrade->OnClick = [&ctx] {
        ctx.logic.buyUpgrade(ctx.state, "up_plastic_1");
    };

    m_actions.add<ui::Spacer>(glm::vec2{0.0f, 4.0f});

    m_soldier = m_actions.add<ui::Button>("Craft soldier(1)");
    m_soldier->OnClick = [&ctx] {
        ctx.logic.craftUnit(ctx.state, "soldier_crooked");
    };

    m_tank = m_actions.add<ui::Button>("Craft tank (2)");
    m_tank->OnClick = [&ctx] {
        ctx.logic.craftUnit(ctx.state, "tank_matchbox");
    };

    m_plane = m_actions.add<ui::Button>("Craft plane (3)");
    m_plane->OnClick = [&ctx] {
        ctx.logic.craftUnit(ctx.state, "plane_paper");
    };

    m_actions.add<ui::Spacer>(glm::vec2{0.0f, 8.0f});

    m_menu = m_actions.add<ui::Button>("Back to menu (Esc)");
    m_menu->OnClick = [&ctx] {
        ctx.scenes.requestReplace(std::make_unique<MainMenuScene>());
    };

    m_hud.clear();
    m_hud.padding = 16.0f;
    m_hud.drawBackground = true;
    m_hud.background = ctx.ui.theme.panelBg;
    m_hud.borderColor = ctx.ui.theme.panelBorder;
    m_hud.borderWidth = 1.0f;
    m_hud.layout = std::make_unique<ui::BoxLayout>(
        ui::MainAxis::Vertical, ui::CrossAlign::Stretch, 6.0f);

    m_resourceLabel = m_hud.add<ui::Label>("...");
    m_resourceLabel->wrap = true;
    m_boss = m_hud.add<ui::Label>("boss: -");
    m_boss->wrap = true;
    m_dps = m_hud.add<ui::Label>("army dps: 0");
    m_dps->wrap = false;
}
void GameScene::refreshLabels(AppContext& ctx) {
    // --- Ресурсы: одна строка на ресурс ---
    {
        const auto& defs = content::Content::instance().allResources();
        const std::size_t n = std::min(defs.size(), ctx.state.resources.size());

        std::string s;
        s.reserve(n * 32u);
        for (std::size_t i = 0; i < n; ++i) {
            if (i > 0) {
                s += '\n';
            }
            char tmp[128];
            std::snprintf(tmp, sizeof(tmp), "%s: %.1f", defs[i].name.c_str(),
                          ctx.state.resources.getByIdx(i));
            s += tmp;
        }
        m_resourceLabel->text = std::move(s);
    }

    // --- Босс ---
    {
        char buf[256];
        std::snprintf(buf, sizeof(buf), "boss: %s\nhp: %.1f / %.1f",
                      ctx.state.currentBoss.name.c_str(),
                      ctx.state.currentBoss.hp, ctx.state.currentBoss.maxHp);
        m_boss->text = buf;
    }

    // --- DPS ---
    {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "army dps: %.1f",
                      ctx.logic.armyDps(ctx.state));
        m_dps->text = buf;
    }
}
}  // namespace game