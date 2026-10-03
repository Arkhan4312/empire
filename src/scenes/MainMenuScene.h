#pragma once
#include "scenes/Scene.h"
#include "ui/Widgets.h"

namespace game {

class MainMenuScene : public Scene {
public:
    MainMenuScene() = default;

    void onEnter(AppContext& ctx) override;
    void update(AppContext& ctx, double dt) override;
    void render(AppContext& ctx) override;

private:
    void buildUi(AppContext& ctx);

    ui::Container m_root;
    ui::Label* m_title = nullptr;
    ui::Label* m_hint = nullptr;
    ui::Button* m_newGame = nullptr;
    ui::Button* m_continue = nullptr;
    ui::Button* m_settings = nullptr;
    ui::Button* m_quit = nullptr;
};
}  // namespace game