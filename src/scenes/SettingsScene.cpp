#include "scenes/SettingsScene.h"

#include <algorithm>
#include <cstdio>
#include <memory>

#include "audio/AudioSystem.h"
#include "core/Input.h"
#include "core/InputSystem.h"
#include "render/Renderer.h"
#include "render/Window.h"
#include "scenes/AppContext.h"
#include "scenes/MainMenuScene.h"
#include "scenes/SceneManager.h"
#include "settings/KeyBindings.h"
#include "settings/Settings.h"
#include "settings/SettingsSystem.h"
#include "ui/UIContext.h"
namespace game {
namespace {
static ui::Label* addSectionHeader(ui::Container& root, const char* text) {
    auto* l = root.add<ui::Label>(text);
    l->scale = 1.05f;
    l->useThemeColor = false;
    l->color = {0.55f, 0.75f, 1.0f, 1.0f};
    return l;
}
}  // namespace
void SettingsScene::onEnter(AppContext& ctx) {
    const std::string base = std::string(EMPIRE_ASSETS_DIR) + "/audio/";
    m_sfxCancel =
        ctx.audio.loadSound("cancel", {base + "sfx/cancel.wav", 0.8f});
    m_uiScaleLocal = ctx.settings.uiScale;
    m_brightnessLocal = ctx.settings.brightness;
    m_pendingAudioDevice = -2;

    m_monitorNames = ctx.window.listMonitors();

    auto devs = ctx.audio.listOutputDevices();
    m_audioNames.clear();
    m_audioNames.emplace_back("Default");
    for (const auto& d : devs) {
        m_audioNames.push_back(d.name);
    }
    buildUi(ctx);
}

void SettingsScene::update(AppContext& ctx, double /*dt*/) {
    if (m_needsRebuild) {
        m_needsRebuild = false;
        ctx.ui.hovered = nullptr;
        ctx.ui.active = nullptr;
        ctx.ui.focused = nullptr;
        buildUi(ctx);
    }
    bool keyBindWaiting = false;
    for (auto* kb : m_keybinds) {
        if (kb == ctx.ui.focused && kb->waitingForKey()) {
            keyBindWaiting = true;
            break;
        }
    }
    if (!keyBindWaiting && ctx.input.state().isKeyPressed(keys::Escape)) {
        ctx.audio.play(m_sfxCancel, 0.9f);
        ctx.scenes.requestReplace(std::make_unique<MainMenuScene>());
        return;
    }
    if (ctx.settings.uiScale != m_uiScaleLocal) {
        ctx.settings.uiScale = m_uiScaleLocal;
        ctx.renderer.setUIScale(m_uiScaleLocal);
    }
    if (ctx.settings.brightness != m_brightnessLocal) {
        ctx.settings.brightness = m_brightnessLocal;
        ctx.renderer.setBrightness(m_brightnessLocal);
    }

    if (m_pendingAudioDevice != -2) {
        const int want = m_pendingAudioDevice - 1;
        m_pendingAudioDevice = -2;
        if (want != ctx.settings.audioDeviceIndex) {
            if (ctx.audio.setOutputDevice(want)) {
                ctx.settings.audioDeviceIndex = want;
            } else {
                std::fprintf(stderr, "[Settings] audio device switch failed\n");
            }
        }
    }

    const glm::vec2 screen = ctx.ui.screenSize();
    const float panelW = 1000.0f;
    const float margin = 16.0f;

    const glm::vec2 avail{panelW, screen.y - margin * 2.0f};
    const glm::vec2 measured = m_viewport.measure(ctx.ui, avail);

    const float panelH = std::min(measured.y, avail.y);
    const float y =
        (measured.y <= avail.y) ? (screen.y - panelH) * 0.5f : margin;
    const float x = (screen.x - panelW) * 0.5f;

    m_viewport.arrange(ctx.ui, {x, y}, {panelW, measured.y});
    m_viewport.update(ctx.ui);
}

void SettingsScene::render(AppContext& ctx) {
    m_viewport.render(ctx.ui);
}

void SettingsScene::buildUi(AppContext& ctx) {
    Settings& s = ctx.settings;

    m_viewport.child = &m_root;
    m_viewport.drawBackground = true;
    m_viewport.background = ctx.ui.theme.panelBg;
    m_viewport.borderColor = ctx.ui.theme.panelBorder;
    m_viewport.borderWidth = 1.0f;
    m_viewport.scrollSpeed = 48.0f;

    m_root.clear();
    m_root.padding = 24.0f;
    m_root.drawBackground = true;
    m_root.borderWidth = 1.0f;
    m_root.layout = std::make_unique<ui::BoxLayout>(
        ui::MainAxis::Vertical, ui::CrossAlign::Stretch, 6.0f);

    auto* title = m_root.add<ui::Label>("Settings");
    title->scale = 1.6f;
    title->halign = ui::Label::HAlign::Center;
    title->valign = ui::Label::VAlign::Middle;

    m_root.add<ui::Divider>();
    // display
    addSectionHeader(m_root, "Display");
    m_windowMode = m_root.add<ui::ChoiceRow>();
    m_windowMode->label = "Window mode";
    m_windowMode->labelWidth = 180.0f;
    m_windowMode->options = {"Windowed", "Borderless", "Fullscreen"};
    static int s_windowModeInt = 0;
    s_windowModeInt = static_cast<int>(s.windowMode);
    m_windowMode->value = &s_windowModeInt;
    m_windowMode->OnValueChanged = [&s](int v) {
        s.windowMode = static_cast<WindowMode>(v);
    };

    m_monitor = m_root.add<ui::ChoiceRow>();
    m_monitor->label = "Monitor";
    m_monitor->labelWidth = 180.0f;
    m_monitor->options = m_monitorNames;
    m_monitor->value = &s.monitorIndex;
    m_root.add<ui::Divider>();
    // Graphics
    addSectionHeader(m_root, "Graphics");
    m_resolution = m_root.add<ui::ChoiceRow>();
    m_resolution->label = "Resolution";
    m_resolution->labelWidth = 180.0f;
    m_resolution->options = {kResolutionNames[0], kResolutionNames[1],
                             kResolutionNames[2]};
    m_resolution->value = &s.resolutionIndex;

    m_quality = m_root.add<ui::ChoiceRow>();
    m_quality->label = "Quality";
    m_quality->labelWidth = 180.0f;
    m_quality->options = {kQualityNames[0], kQualityNames[1], kQualityNames[2],
                          kQualityNames[3]};
    m_quality->value = &s.qualityIndex;

    m_uiScale = m_root.add<ui::Slider>();
    m_uiScale->label = "UI Scale";
    m_uiScale->value = &m_uiScaleLocal;
    m_uiScale->minValue = 0.75f;
    m_uiScale->maxValue = 2.00f;
    m_uiScale->step = 0.10f;

    m_brightness = m_root.add<ui::Slider>();
    m_brightness->label = "Brightness";
    m_brightness->value = &m_brightnessLocal;
    m_brightness->minValue = 0.5f;
    m_brightness->maxValue = 1.5f;
    m_brightness->step = 0.05f;

    m_vsync = m_root.add<ui::Checkbox>("V-Sync");
    m_vsync->value = &s.vsync;

    m_showFPS = m_root.add<ui::Checkbox>("Show FPS");
    m_showFPS->value = &s.showFPS;

    m_root.add<ui::Divider>();
    // Systems
    addSectionHeader(m_root, "System");
    m_gpuLabel = m_root.add<ui::Label>("GPU: ...");
    m_gpuLabel->useThemeColor = false;
    m_gpuLabel->color = {0.75f, 0.78f, 0.85f, 1.0f};
    m_gpuLabel->text = "GPU: " + ctx.window.gpuName();

    m_glLabel = m_root.add<ui::Label>("OpenGL: ...");
    m_glLabel->useThemeColor = false;
    m_glLabel->color = {0.75f, 0.78f, 0.85f, 1.0f};
    m_glLabel->text = "OpenGL: " + ctx.window.glVersion();
    m_root.add<ui::Divider>();
    // Audio
    addSectionHeader(m_root, "Audio");
    m_audioDevice = m_root.add<ui::ChoiceRow>();
    m_audioDevice->label = "Output";
    m_audioDevice->labelWidth = 180.0f;
    m_audioDevice->options = m_audioNames;
    static int s_audioIdx = 0;
    s_audioIdx = s.audioDeviceIndex + 1;
    m_audioDevice->value = &s_audioIdx;
    m_audioDevice->OnValueChanged = [this](int v) { m_pendingAudioDevice = v; };

    m_master = m_root.add<ui::Slider>();
    m_master->label = "Master";
    m_master->value = &s.masterVolume;
    m_master->minValue = 0.0f;
    m_master->maxValue = 1.0f;

    m_sfx = m_root.add<ui::Slider>();
    m_sfx->label = "SFX";
    m_sfx->value = &s.sfxVolume;
    m_sfx->minValue = 0.0f;
    m_sfx->maxValue = 1.0f;

    m_music = m_root.add<ui::Slider>();
    m_music->label = "Music";
    m_music->value = &s.musicVolume;
    m_music->minValue = 0.0f;
    m_music->maxValue = 1.0f;

    m_root.add<ui::Divider>();

    // Controls
    addSectionHeader(m_root, "Controls");

    m_keybinds.clear();
    m_keybinds.reserve(s.keys.size());
    for (auto& kb : s.keys) {
        auto* row =
            m_root.add<ui::KeyBindButton>(kb.action, kb.key, kb.defaultKey);
        row->onKeyChanged = [&s, action = kb.action](int newKey) {
            for (auto& e : s.keys) {
                if (e.action == action) {
                    e.key = newKey;
                    break;
                }
            }
        };
        m_keybinds.push_back(row);
    }

    m_root.add<ui::Divider>();
    // Nav
    addSectionHeader(m_root, "Danger zone");
    m_resetDefaults = m_root.add<ui::Button>("Reset settings to defaults");
    m_resetDefaults->OnClick = [this, &ctx] {
        ctx.settings = Settings::defaults();
        m_uiScaleLocal = ctx.settings.uiScale;
        m_brightnessLocal = ctx.settings.brightness;
        ctx.renderer.setUIScale(ctx.settings.uiScale);
        ctx.renderer.setBrightness(ctx.settings.brightness);
        m_pendingAudioDevice = -2;
        m_needsRebuild = true;
    };

    m_deleteSave = m_root.add<ui::Button>("Delete save file");
    m_deleteSave->OnClick = [this, &ctx] {
        if (settings::removeFile(ctx.savePath)) {
            std::fprintf(stderr, "[Settings] save file removed: %s\n",
                         ctx.savePath);
        }
    };

    m_root.add<ui::Divider>();
    m_back = m_root.add<ui::Button>("Back");
    m_back->OnClick = [this, &ctx] {
        ctx.audio.play(m_sfxCancel, 0.9f);
        ctx.scenes.requestReplace(std::make_unique<MainMenuScene>());
    };
}
}  // namespace game