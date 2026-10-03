#pragma once
#include <vector>

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

    ui::Container m_root;

    // Graphics
    ui::Checkbox* m_vsync = nullptr;
    ui::Checkbox* m_fullscreen = nullptr;
    ui::Checkbox* m_showFPS = nullptr;
    ui::ChoiceRow* m_resolution = nullptr;
    ui::ChoiceRow* m_quality = nullptr;

    // Audio
    ui::Slider* m_master = nullptr;
    ui::Slider* m_sfx = nullptr;
    ui::Slider* m_music = nullptr;

    // Controls
    std::vector<ui::KeyBindButton*> m_keybinds;
    // Nav
    ui::Button* m_back = nullptr;
};
}  // namespace game