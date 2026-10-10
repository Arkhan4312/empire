#include "scenes/MainMenuScene.h"

#include <fstream>
#include <memory>
#include <string>

#include "content/Content.h"
#include "core/GameLogic.h"
#include "core/GameState.h"
#include "core/InputSystem.h"
#include "render/Renderer.h"
#include "render/Window.h"
#include "scenes/AppContext.h"
#include "scenes/GameScene.h"
#include "scenes/SceneManager.h"
#include "scenes/SettingsScene.h"
#include "systems/SaveSystem.h"
#include "ui/UIContext.h"
#include "util/Random.h"

namespace game {
static bool saveExists(const char* path) {
    std::ifstream f(path);
    return f.good();
}

void MainMenuScene::onEnter(AppContext& ctx) {
    const std::string base = std::string(EMPIRE_ASSETS_DIR) + "/audio/";
    m_sfxClickOn = ctx.audio.loadSound("click_double_on",
                                       {base + "sfx/click_double_on.wav"});
    m_sfxClickOff = ctx.audio.loadSound("click_double_off",
                                        {base + "sfx/click_double_off.wav"});
    m_sfxBookClose =
        ctx.audio.loadSound("book_close", {base + "sfx/book_close.wav"});
    m_sfxCancel = ctx.audio.loadSound("cancel", {base + "sfx/cancel.wav"});

    ctx.audio.playMusic(base + "music/main_menu(tmp).wav", 0.8f, true);

    buildUi(ctx);
}

void MainMenuScene::update(AppContext& ctx, double dt) {
    const glm::vec2 screen = ctx.ui.screenSize();


    const glm::vec2 avail{460.0f, 340.0f};
    const glm::vec2 measured = m_root.measure(ctx.ui, avail);

    const glm::vec2 pos{(screen.x - measured.x) * 0.5f,
                        (screen.y - measured.y) * 0.5f};
    m_root.arrange(ctx.ui, pos, measured);
    m_root.update(ctx.ui);

    const auto& in = ctx.input.state();
    if (in.isMousePressed(mouse::Left) && ctx.ui.hovered == nullptr) {
        playEmptyClick(ctx);
    }
}

void MainMenuScene::render(AppContext& ctx) {
    m_root.render(ctx.ui);
}

void MainMenuScene::buildUi(AppContext& ctx) {
    m_root.clear();
    m_root.padding = 24.0f;
    m_root.drawBackground = true;
    m_root.background = ctx.ui.theme.panelBg;
    m_root.borderColor = ctx.ui.theme.panelBorder;
    m_root.borderWidth = 1.0f;
    m_root.layout = std::make_unique<ui::BoxLayout>(
        ui::MainAxis::Vertical, ui::CrossAlign::Stretch, 10.0f);

    m_title = m_root.add<ui::Label>("Empire from Nothing");
    m_title->scale = 1.8f;
    m_title->halign = ui::Label::HAlign::Center;

    m_hint = m_root.add<ui::Label>("Idle / clicker prototype");
    m_hint->halign = ui::Label::HAlign::Center;
    m_hint->useThemeColor = false;
    m_hint->color = {0.65f, 0.68f, 0.75f, 1.0f};

    m_root.add<ui::Spacer>(glm::vec2{0.0f, 18.0f});

    const bool hasSave = saveExists(ctx.savePath);

    m_newGame = m_root.add<ui::Button>("New game");
    m_newGame->OnClick = [this, &ctx] {
        ctx.audio.play(m_sfxClickOn, 0.8f);
        content::initNewGame(ctx.state);
        ctx.state.lastSaveTimestamp = SaveSystem::nowSeconds();
        ctx.scenes.requestReplace(std::make_unique<GameScene>());
    };

    m_continue = m_root.add<ui::Button>("Continue");
    m_continue->enabled = hasSave;
    m_continue->OnClick = [this, &ctx] {
        ctx.audio.play(m_sfxClickOn, 0.8f);
        if (!ctx.save.load(ctx.state, ctx.savePath)) {
            return;
        }
        const auto away = SaveSystem::computeOfflineSeconds(ctx.state, 0);
        if (away > 0) {
            SaveSystem::applyOffline(ctx.state, ctx.logic, away);
        }
        ctx.scenes.requestReplace(std::make_unique<GameScene>());
    };

    m_settings = m_root.add<ui::Button>("Settings");
    m_settings->OnClick = [this, &ctx] {
        ctx.audio.play(m_sfxBookClose, 0.9f);
        ctx.scenes.requestReplace(std::make_unique<SettingsScene>());
    };

    m_root.add<ui::Spacer>(glm::vec2{0.0f, 4.0f});

    m_quit = m_root.add<ui::Button>("Quit");
    m_quit->OnClick = [this, &ctx] {
        ctx.audio.play(m_sfxCancel, 0.9f);
        ctx.window.requestClose();
    };
}

void MainMenuScene::playEmptyClick(AppContext& ctx) {
    if (Rng::frand(0.0f, 1.0f) < 0.5f) {
        ctx.audio.play(m_sfxClickOff, 0.6f);
    } else {
        ctx.audio.play(m_sfxClickOn, 0.6f);
    }
}

}  // namespace game