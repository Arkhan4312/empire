#include <chrono>
#include <cstdint>
#include <cstdio>
#include <memory>

#include "audio/AudioSystem.h"
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
#include "settings/SettingsSystem.h"
#include "settings/WindowMode.h"
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
    const char* kSettingsPath = "settings.json";
    GameState state;
    GameLogic logic;
    SaveSystem save;
    Settings settings;
    settings::load(settings, kSettingsPath);

    audio::AudioSystem audio;
    if (!audio.init()) {
        std::fprintf(stderr, "[Init] audio disabled\n");
    }

    {
        std::string err;
        if (!content::Content::instance().loadFromDirectory(
                std::string(EMPIRE_ASSETS_DIR) + "/content", &err)) {
            std::fprintf(stderr, "[Content] load failed: %s\n", err.c_str());
            std::fprintf(stderr, "[Content] using built-it fallback\n");
        }
    }

    content::initNewGame(state);
    state.lastSaveTimestamp = SaveSystem::nowSeconds();

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
    window.applyVsync(settings.vsync);
    window.applyWindowMode(
        settings.windowMode, kResolutions[settings.resolutionIndex].w,
        kResolutions[settings.resolutionIndex].h, settings.monitorIndex);
    renderer.setUIScale(settings.uiScale);
    renderer.setBrightness(settings.brightness);

    render::Font font;
    const bool fontOk = font.loadFromFile(
        std::string(EMPIRE_ASSETS_DIR) + "/fonts/Cyrillic.ttf", 22.0f);
    if (!fontOk) {
        std::fprintf(stderr, "[Init] font not  loaded, running without text\n");
    }

    ui::UIContext uiCtx;
    uiCtx.renderer = &renderer;
    uiCtx.font = fontOk ? &font : nullptr;
    uiCtx.input = &input.state();

    SceneManager scenes;
    AppContext ctx{
        state,
        logic,
        save,
        settings,
        window,
        renderer,
        input,
        uiCtx,
        scenes,
        audio,
        fontOk ? &font : nullptr,
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

#if !defined(NDEBUG)
        if (input.state().isKeyPressed(301)) {
            std::string err;
            if (!content::Content::instance().reload(&err)) {
                std::fprintf(stderr, "[HotReload] failed %s\n", err.c_str());
            } else {
                std::fprintf(stderr, "[HotReload] content reloaded\n");
            }
        }
#endif

        if (SaveSystem::needAutosave(state)) {
            if (!save.save(state, kSavePath)) {
                std::fprintf(stderr, "[Autosave] failed to write '%s'\n",
                             kSavePath);
            }
        }

        uiCtx.beginFrame();
        if (Scene* s = scenes.current()) {
            s->update(ctx, dt);
        }
        scenes.applyPending(ctx);

        audio.setMasterVolume(settings.masterVolume);
        audio.setSfxVolume(settings.sfxVolume);
        audio.setMusicVolume(settings.musicVolume);

        static Settings appliedSnapshot = settings;
        if (appliedSnapshot.vsync != settings.vsync) {
            window.applyVsync(settings.vsync);
        }
        if (appliedSnapshot.windowMode != settings.windowMode ||
            appliedSnapshot.monitorIndex != settings.monitorIndex ||
            appliedSnapshot.resolutionIndex != settings.resolutionIndex) {
            window.applyWindowMode(settings.windowMode,
                                   kResolutions[settings.resolutionIndex].w,
                                   kResolutions[settings.resolutionIndex].h,
                                   settings.monitorIndex);
            window.applyVsync(settings.vsync);
        }
        if (appliedSnapshot.uiScale != settings.uiScale) {
            renderer.setUIScale(settings.uiScale);
        }
        if (appliedSnapshot.brightness != settings.brightness) {
            renderer.setBrightness(settings.brightness);
        }
        appliedSnapshot = settings;
        audio.update(static_cast<float>(dt));

        renderer.beginFrame();
        if (Scene* s = scenes.current()) {
            s->render(ctx);
        }
        if (settings.showFPS && fontOk) {
            static double fpsAccum = 0.0;
            static int fpsFrames = 0;
            static float fpsValue = 0.0f;

            fpsAccum += dt;
            ++fpsFrames;
            if (fpsAccum >= 0.5) {
                fpsValue = static_cast<float>(fpsFrames / fpsAccum);
                fpsAccum = 0.0;
                fpsFrames = 0;
            }

            char buf[64];
            std::snprintf(buf, sizeof(buf), "FPS: %.1f", fpsValue);
            renderer.drawText(font, buf, {8.0f, 8.0f}, {0.9f, 0.9f, 0.9f, 0.6f},
                              1.0f);
        }
        renderer.endFrame();

        uiCtx.endFrame();
        window.swapBuffers();
    }
    settings::save(settings, kSettingsPath);
    save.save(state, kSavePath);

    audio.shutdown();
    font.destroy();
    input.detach();
    renderer.shutdown();
    window.destroy();
    return 0;
}