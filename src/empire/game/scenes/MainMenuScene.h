#pragma once
#include <empire/engine/audio/AudioSystem.h>
#include <empire/game/scenes/Scene.h>
#include <empire/engine/ui/Widgets.h>

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

    empire::audio::SoundId m_sfxClickOn = empire::audio::kInvalidSound;
    empire::audio::SoundId m_sfxClickOff = empire::audio::kInvalidSound;
    empire::audio::SoundId m_sfxBookClose = empire::audio::kInvalidSound;
    empire::audio::SoundId m_sfxCancel = empire::audio::kInvalidSound;
};
}  // namespace game