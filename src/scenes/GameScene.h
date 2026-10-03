#pragma once
#include "scenes/Scene.h"
#include "ui/Widgets.h"

namespace game {

class GameScene : public Scene {
public:
    GameScene() = default;

    void onEnter(AppContext& ctx) override;
    void onExit(AppContext& ctx) override;
    void update(AppContext& ctx, double dt) override;
    void render(AppContext& ctx) override;

private:
    void buildUi(AppContext& ctx);
    void refreshLabels(AppContext& ctx);

    ui::Container m_hud;
    ui::Label* m_plastic = nullptr;
    ui::Label* m_boss = nullptr;
    ui::Label* m_dps = nullptr;

    ui::Container m_actions;
    ui::Button* m_click = nullptr;
    ui::Button* m_upgrade = nullptr;
    ui::Button* m_soldier = nullptr;
    ui::Button* m_tank = nullptr;
    ui::Button* m_plane = nullptr;
    ui::Button* m_menu = nullptr;
};
}  // namespace game