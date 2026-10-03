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
        const glm::vec2 measured = m_hud.measure(ctx.ui, avail);
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

    m_plastic = m_hud.add<ui::Label>("plastic: 0");
    m_boss = m_hud.add<ui::Label>("boss: -");
    m_dps = m_hud.add<ui::Label>("army dps: 0");
}
void GameScene::refreshLabels(AppContext& ctx) {
    char buf[256];
    std::snprintf(buf, sizeof(buf), "plastic: %.1f",
                  ctx.state.resources.get(ResourceType::PLASTIC));
    m_plastic->text = buf;

    std::snprintf(buf, sizeof(buf), "boss: %s hp: %.1f / %.1f",
                  ctx.state.currentBoss.name.c_str(), ctx.state.currentBoss.hp,
                  ctx.state.currentBoss.maxHp);
    m_boss->text = buf;

    double dps = 0.0f;
    for (const auto& s : ctx.state.units) {
        if (const UnitDef* d = content::findUnit(s.id)) {
            dps += d->damage * static_cast<double>(s.count) *
                   ctx.state.qualityMult;
        }
    }
    std::snprintf(buf, sizeof(buf), "army dps: %.1f", dps);
    m_dps->text = buf;
}
}  // namespace game