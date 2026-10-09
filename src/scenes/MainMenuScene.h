#pragma once
#include "audio/AudioSystem.h"
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

    void playEmptyClick(AppContext& ctx);

    ui::Container m_root;
    ui::Label* m_title = nullptr;
    ui::Label* m_hint = nullptr;
    ui::Button* m_newGame = nullptr;
    ui::Button* m_continue = nullptr;
    ui::Button* m_settings = nullptr;
    ui::Button* m_quit = nullptr;

    audio::SoundId m_sfxClickOn = audio::kInvalidSound;
    audio::SoundId m_sfxClickOff = audio::kInvalidSound;
    audio::SoundId m_sfxBookClose = audio::kInvalidSound;
    audio::SoundId m_sfxCancel = audio::kInvalidSound;
};
}  // namespace game