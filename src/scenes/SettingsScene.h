#pragma once
#include <vector>

#include "audio/AudioSystem.h"
#include "scenes/Scene.h"
#include "ui/Widgets.h"

namespace game {

class SettingsScene : public Scene {
public:
    void onEnter(AppContext& ctx) override;
    void update(AppContext& ctx, double dt) override;
    void render(AppContext& ctx) override;

private:
    void buildUi(AppContext& ctx);

    ui::ScrollView m_viewport;
    ui::Container m_root;

    // Graphics
    ui::ChoiceRow* m_windowMode = nullptr;
    ui::ChoiceRow* m_monitor = nullptr;
    ui::ChoiceRow* m_resolution = nullptr;
    ui::ChoiceRow* m_quality = nullptr;
    ui::Slider* m_uiScale = nullptr;
    ui::Slider* m_brightness = nullptr;
    ui::Checkbox* m_vsync = nullptr;
    ui::Checkbox* m_showFPS = nullptr;

    ui::Label* m_gpuLabel = nullptr;
    ui::Label* m_glLabel = nullptr;
    // Audio
    ui::ChoiceRow* m_audioDevice = nullptr;
    ui::Slider* m_master = nullptr;
    ui::Slider* m_sfx = nullptr;
    ui::Slider* m_music = nullptr;
    // Controls
    std::vector<ui::KeyBindButton*> m_keybinds;
    // Nav
    ui::Button* m_back = nullptr;
    ui::Button* m_resetDefaults = nullptr;
    ui::Button* m_deleteSave = nullptr;

    float m_uiScaleLocal = 1.0f;
    float m_brightnessLocal = 1.0f;

    int m_pendingAudioDevice = -2;
    audio::SoundId m_sfxCancel = audio::kInvalidSound;

    bool m_needsRebuild = false;
    std::vector<std::string> m_monitorNames;
    std::vector<std::string> m_audioNames;
};
}  // namespace game