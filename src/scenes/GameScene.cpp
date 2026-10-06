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
#include "util/Random.h"

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

    drainEvents(ctx);

    const InputState& in = ctx.input.state();
    if (in.isKeyPressed(keys::Space)) {
        const double gain = ctx.logic.clickPower(ctx.state);
        ctx.logic.clickMain(ctx.state);

        char buf[32];
        std::snprintf(buf, sizeof(buf), "+%.1f", gain);
        spawnFloater(ctx, buf, {0.65f, 0.9f, 1.0f, 1.0f});
    }
    if (in.isKeyPressed(keys::E)) {
        ctx.logic.buyUpgrade(ctx.state, "up_plastic_1");
    }
    // units
    if (in.isKeyPressed(49)) {
        ctx.logic.craftUnit(ctx.state, "soldier_crooked");
    }
    if (in.isKeyPressed(50)) {
        ctx.logic.craftUnit(ctx.state, "tank_matchbox");
    }
    if (in.isKeyPressed(51)) {
        ctx.logic.craftUnit(ctx.state, "plane_paper");
    }
    // buildings
    if (in.isKeyPressed(52)) {
        ctx.logic.build(ctx.state, "paper_mill");
    }
    if (in.isKeyPressed(53)) {
        ctx.logic.build(ctx.state, "glue_pot");
    }
    if (in.isKeyPressed(54)) {
        ctx.logic.build(ctx.state, "plastic_recycler");
    }
    // upgrades
    if (in.isKeyPressed(keys::U)) {
        ctx.logic.buyCheapestUpgrade(ctx.state);
    }
    if (in.isKeyPressed(keys::Escape)) {
        ctx.scenes.requestReplace(std::make_unique<MainMenuScene>());
        return;
    }
    updateTransient(static_cast<float>(dt));
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
    renderTransient(ctx);
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
    m_click->OnClick = [this, &ctx] {
        const double gain = ctx.logic.clickPower(ctx.state);
        ctx.logic.clickMain(ctx.state);
        char buf[32];
        std::snprintf(buf, sizeof(buf), "+%.1f", gain);
        spawnFloater(ctx, buf, {0.65f, 0.9f, 1.0f, 1.0f});
    };

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

    m_actions.add<ui::Spacer>(glm::vec2{0.0f, 4.0f});

    auto* b1 = m_actions.add<ui::Button>("Build paper mill (4)");
    b1->OnClick = [&ctx] { ctx.logic.build(ctx.state, "paper_mill"); };
    auto* b2 = m_actions.add<ui::Button>("Build glue pot (5)");
    b2->OnClick = [&ctx] { ctx.logic.build(ctx.state, "glue_pot"); };
    auto* b3 = m_actions.add<ui::Button>("Build recycler (6)");
    b3->OnClick = [&ctx] { ctx.logic.build(ctx.state, "plastic_recycler"); };

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
    {
        const auto& defs = content::Content::instance().allResources();
        const std::size_t n = std::min(defs.size(), ctx.state.resources.size());

        std::string s;
        s.reserve(n * 40u);
        for (std::size_t i = 0; i < n; ++i) {
            if (i > 0) {
                s += '\n';
            }
            char tmp[160];
            std::snprintf(tmp, sizeof(tmp), "%s: %.1f (+%.1f/s)",
                          defs[i].name.c_str(), ctx.state.resources.getByIdx(i),
                          ctx.state.resources.rateAt(i));
            s += tmp;
        }
        m_resourceLabel->text = std::move(s);
    }

    {
        char buf[256];
        std::snprintf(buf, sizeof(buf), "boss: %s\nhp: %.1f / %.1f",
                      ctx.state.currentBoss.name.c_str(),
                      ctx.state.currentBoss.hp, ctx.state.currentBoss.maxHp);
        m_boss->text = buf;
    }

    {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "army dps: %.1f",
                      ctx.logic.armyDps(ctx.state));
        m_dps->text = buf;
    }
    if (m_tank) {
        m_tank->enabled = ctx.state.isUnlocked("tank_matchbox");
    }
    if (m_plane) {
        m_plane->enabled = ctx.state.isUnlocked("plane_paper");
    }
}
void GameScene::spawnFloater(AppContext& ctx, std::string text,
                             glm::vec4 color) {
    FloatingText f;
    f.text = std::move(text);
    f.color = color;

    const glm::vec2 m = ctx.input.state().mousePos;
    f.pos = {m.x + Rng::frand(-12.0f, 12.0f), m.y + Rng::frand(-8.0f, 8.0f)};
    f.velocity = {Rng::frand(-10.0f, 10.0f), -60.0f};
    f.life = 1.1f;
    f.scale = 1.35f;

    m_floaters.push_back(std::move(f));
}

void GameScene::spawnNotification(std::string text, glm::vec4 color) {
    Notification n;
    n.text = std::move(text);
    n.color = color;
    n.life = 3.2f;
    m_notifications.push_back(std::move(n));
}

void GameScene::drainEvents(AppContext& ctx) {
    for (const auto& e : ctx.state.events) {
        switch (e.kind) {
            case GameEventKind::Unlock:
                spawnNotification("Открыто: " + e.name,
                                  {0.55f, 0.95f, 0.55f, 1.0f});
                break;
            case GameEventKind::BossKilled:
                spawnNotification("Победа: " + e.name,
                                  {0.95f, 0.55f, 0.55f, 1.0f});
                break;
        }
    }
    ctx.state.events.clear();
}
void GameScene::updateTransient(double dt) {
    const float fdt = static_cast<float>(dt);

    for (auto& f : m_floaters) {
        f.age += fdt;
        f.pos += f.velocity * fdt;
        f.velocity.y *= 0.94f;
        f.velocity.x *= 0.94f;
    }
    m_floaters.erase(
        std::remove_if(m_floaters.begin(), m_floaters.end(),
                       [](const FloatingText& f) { return f.age >= f.life; }),
        m_floaters.end());
    for (auto& n : m_notifications) {
        n.age + fdt;
    }
    m_notifications.erase(
        std::remove_if(m_notifications.begin(), m_notifications.end(),
                       [](const Notification& n) { return n.age >= n.life; }),
        m_notifications.end());
}

void GameScene::renderTransient(AppContext& ctx) {
    if (!ctx.font) {
        return;
    }
    render::Renderer& r = ctx.renderer;

    for (const auto& f : m_floaters) {
        const float t = f.age / f.life;
        const float alpha = (t < 0.55f) ? 1.0f : (1.0f - (t - 0.55f) / 0.45f);
        glm::vec4 c = f.color;
        c.a *= alpha;

        const float w = ctx.font->measureText(f.text) * f.scale;
        const glm::vec2 p = {f.pos.x - w * 0.5f, f.pos.y};

        glm::vec4 shadow = {0.0f, 0.0f, 0.0f, 0.6f * alpha};
        r.drawText(*ctx.font, f.text, p + glm::vec2{1.0f, 1.0f}, shadow,
                   f.scale);
        r.drawText(*ctx.font, f.text, p, c, f.scale);
    }

    const float screenW = r.screenSize().x;
    float y = 70.0f;
    for (const auto& n : m_notifications) {
        const float t = n.age / n.life;
        const float alpha = (t < 0.75f) ? 1.0f : (1.0f - (t - 0.75f) / 0.25f);

        const float scale = 1.6f;
        const float w = ctx.font->measureText(n.text) * scale;
        const glm::vec2 p = {(screenW - w) * 0.5f, y};

        const glm::vec4 bg = {0.10f, 0.11f, 0.14f, 0.88f * alpha};
        const glm::vec4 brd = {0.30f, 0.34f, 0.40f, 0.95f * alpha};
        const float boxH = 40.0f;
        r.drawRect({p.x - 16.0f, p.y - 8.0f}, {w + 32.0f, boxH}, bg);
        r.drawRect({p.x - 16.0f, p.y - 8.0f}, {w + 32.0f, 2.0f}, brd);
        r.drawRect({p.x - 16.0f, p.y - 8.0f + boxH - 2.0f}, {w + 32.0f, 2.0f},
                   brd);

        glm::vec4 c = n.color;
        c.a *= alpha;
        r.drawText(*ctx.font, n.text, p, c, scale);

        y += 48.0f;
    }
}
}  // namespace game