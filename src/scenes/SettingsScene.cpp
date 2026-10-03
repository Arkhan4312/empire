#include "scenes/SettingsScene.h"

#include <algorithm>
#include <memory>

#include "core/Input.h"
#include "core/InputSystem.h"
#include "render/Renderer.h"
#include "render/Window.h"
#include "scenes/AppContext.h"
#include "scenes/MainMenuScene.h"
#include "scenes/SceneManager.h"
#include "settings/Settings.h"
#include "ui/UIContext.h"

namespace game {

static ui::Label* addSectionHeader(ui::Container& root, const char* text) {
    auto* l = root.add<ui::Label>(text);
    l->scale = 1.05f;
    l->useThemeColor = false;
    l->color = {0.55f, 0.75f, 1.0f, 1.0f};
    return l;
}

void SettingsScene::onEnter(AppContext& ctx) {
    buildUi(ctx);
}

void SettingsScene::update(AppContext& ctx, double /*dt*/) {
    if (ctx.input.state().isKeyPressed(keys::Escape)) {
        ctx.scenes.requestReplace(std::make_unique<MainMenuScene>());
        return;
    }

    int w = 0;
    int h = 0;
    ctx.window.framebufferSize(w, h);
    const float panelW = 560.0f;
    const float margin = 16.0f;

    const glm::vec2 avail{ panelW, static_cast<float>(h) - margin * 2.0f };
    const glm::vec2 measured = m_root.measure(ctx.ui, avail);

    const float panelH = std::min(measured.y, avail.y);
    const float y = (measured.y <= avail.y) ? (static_cast<float>(h) - panelH) * 0.5f : margin;
    const float x = (static_cast<float>(w) - panelW) * 0.5f;

    m_root.arrange(ctx.ui, { x,margin }, { panelW, panelH });
    m_root.update(ctx.ui);
}

void SettingsScene::render(AppContext& ctx) {
    m_root.render(ctx.ui);
}

void SettingsScene::buildUi(AppContext& ctx) {
    Settings& s = ctx.settings;

    m_root.clear();
    m_root.padding = 24.0f;
    m_root.drawBackground = true;
    m_root.background = ctx.ui.theme.panelBg;
    m_root.borderColor = ctx.ui.theme.panelBorder;
    m_root.borderWidth = 1.0f;
    m_root.layout = std::make_unique<ui::BoxLayout>(
        ui::MainAxis::Vertical, ui::CrossAlign::Stretch, 6.0f);

    auto* title = m_root.add<ui::Label>("Settings");
    title->scale = 1.6f;
    title->halign = ui::Label::HAlign::Center;
    title->valign = ui::Label::VAlign::Middle;

    m_root.add<ui::Divider>();

    // Graphics
    addSectionHeader(m_root, "Graphics");

    m_vsync = m_root.add<ui::Checkbox>("V-Sync");
    m_vsync->value = &s.vsync;

    m_fullscreen = m_root.add<ui::Checkbox>("Fullscreen");
    m_fullscreen->value = &s.fullscreen;

    m_showFPS = m_root.add<ui::Checkbox>("Show FPS");
    m_showFPS->value = &s.showFPS;

    m_resolution = m_root.add<ui::ChoiceRow>();
    m_resolution->label = "Resolution";
    m_resolution->options = {kResolutionNames[0], kResolutionNames[1],
                             kResolutionNames[2]};
    m_resolution->value = &s.resolutionIndex;

    m_quality = m_root.add<ui::ChoiceRow>();
    m_quality->label = "Quality";
    m_quality->options = {kQualityNames[0], kQualityNames[1], kQualityNames[2],
                          kQualityNames[3]};
    m_quality->value = &s.qualityIndex;

    m_root.add<ui::Divider>();

    // Audio
    addSectionHeader(m_root, "Audio");

    m_master = m_root.add<ui::Slider>();
    m_master->label = "Master";
    m_master->value = &s.masterVoume;

    m_sfx = m_root.add<ui::Slider>();
    m_sfx->label = "SFX";
    m_sfx->value = &s.sfxVolume;

    m_music = m_root.add<ui::Slider>();
    m_music->label = "Music";
    m_music->value = &s.musicVolume;

    m_root.add<ui::Divider>();

    // Controls
    addSectionHeader(m_root, "Controls");

    m_keybinds.clear();
    m_keybinds.reserve(s.keys.size());
    for (const auto& kb : s.keys) {
        auto* row = m_root.add<ui::KeyBindButton>(kb.action, kb.key);
        m_keybinds.push_back(row);
    }

    m_root.add<ui::Divider>();
    // Nav
    m_back = m_root.add<ui::Button>("Back");
    m_back->OnClick = [&ctx] {
        ctx.scenes.requestReplace(std::make_unique<MainMenuScene>());
    };
}
}  // namespace game