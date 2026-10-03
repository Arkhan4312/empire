#include <chrono>
#include <cstdint>
#include <cstdio>
#include <memory>

#include "content/Content.h"
#include "core/GameLogic.h"
#include "core/GameState.h"
#include "core/Input.h"
#include "core/InputSystem.h"
#include "render/Font.h"
#include "render/Renderer.h"
#include "render/Window.h"
#include "scenes/AppContext.h"
#include "scenes/MainMenuScene.h"
#include "scenes/SceneManager.h"
#include "settings/Settings.h"
#include "systems/SaveSystem.h"
#include "ui/UIContext.h"

using namespace game;

static std::int64_t nowSeconds() {
    using namespace std::chrono;
    return duration_cast<seconds>(system_clock::now().time_since_epoch())
        .count();
}

int main() {
    const char* kSavePath = "save.json";

    GameState state;
    GameLogic logic;
    SaveSystem save;
    Settings settings;
    content::initNewGame(state);

    render::Window window;
    if (!window.create(1280, 720, "Empire from Nothing")) {
        return 1;
    }

    InputSystem input;
    input.attach(window.handle());

    render::Renderer renderer;
    if (!renderer.init(window)) {
        window.destroy();
        return 1;
    }

    render::Font font;
    const bool fontOk = font.loadFromFile(
        std::string(EMPIRE_ASSETS_DIR) + "/fonts/main.ttf", 22.0f);
    if (!fontOk) {
        std::fprintf(stderr, "[Init] font not  loaded, running without text\n");
    }

    ui::UIContext uiCtx;
    uiCtx.renderer = &renderer;
    uiCtx.font = fontOk ? &font : nullptr;
    uiCtx.input = &input.state();

    SceneManager scenes;
    AppContext ctx{
        state,     logic, save,  settings, window,
        renderer,  input, uiCtx, scenes,   fontOk ? &font : nullptr,
        kSavePath,
    };

    scenes.replace(std::make_unique<MainMenuScene>(), ctx);

    double lastTime = window.time();

    while (!window.shouldClose()) {
        input.beginFrame();
        window.pollEvents();

        const double now = window.time();
        double dt = now - lastTime;
        lastTime = now;
        if (dt > 0.25) {
            dt = 0.25;
        }

        uiCtx.beginFrame();
        if (Scene* s = scenes.current()) {
            s->update(ctx, dt);
        }
        scenes.applyPending(ctx);

        renderer.beginFrame();
        if (Scene* s = scenes.current()) {
            s->render(ctx);
        }
        renderer.endFrame();

        uiCtx.endFrame();
        window.swapBuffers();
    }

    state.lastSaveTimestamp = nowSeconds();
    save.save(state, kSavePath);

    font.destroy();
    input.detach();
    renderer.shutdown();
    window.destroy();
    return 0;
}