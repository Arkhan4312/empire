#include <empire/engine/audio/AudioSystem.h>
#include <empire/engine/core/Input.h>
#include <empire/engine/core/InputSystem.h>
#include <empire/engine/log/Logger.h>
#include <empire/engine/render/Font.h>
#include <empire/engine/render/Renderer.h>
#include <empire/engine/render/Window.h>
#include <empire/engine/settings/Settings.h>
#include <empire/engine/settings/SettingsSystem.h>
#include <empire/engine/settings/WindowMode.h>
#include <empire/engine/ui/UIContext.h>
#include <empire/engine/util/AssetPath.h>
#include <empire/game/content/Content.h>
#include <empire/game/logic/GameLogic.h>
#include <empire/game/save/SaveSystem.h>
#include <empire/game/scenes/AppContext.h>
#include <empire/game/scenes/MainMenuScene.h>
#include <empire/game/scenes/SceneManager.h>
#include <empire/game/state/GameState.h>

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <memory>

#if defined(_WIN32)
#include <Windows.h>
#endif

using namespace game;

static std::int64_t nowSeconds() {
    using namespace std::chrono;
    return duration_cast<seconds>(system_clock::now().time_since_epoch())
        .count();
}

int main() {
#if defined(_WIN32)
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    empire::asset::setRoot(std::filesystem::path(EMPIRE_ASSETS_DIR));
    using namespace empire::log;
    DefaultSinkFactory sinks;
    LoggerConfig logCfg{
        .consoleEnabled = true,
        .consoleLevel = Level::Info,
        .colorConsole = true,
        .fileEnabled = true,
        .fileLevel = Level::Debug,
        .logFilePath = "logs/empire.log",
        .truncate = false,
        .format =
            {
                .timestamps = true,
                .sourceLocation = true,
                .channel = true,
            },
        .channelLevels = {},
        .defaultChannelLevel = Level::Info,
    };
    auto logger = buildLogger(sinks, logCfg);
    setDefaultLogger(logger.get());

    GAME_LOG_INFO("Empire from Nothing v{} starting", "0.1.0");

    const char* kSavePath = "save.json";
    const char* kSettingsPath = "settings.json";
    GameState state;
    GameLogic logic;
    SaveSystem save;
    Settings settings;
    settings::load(settings, kSettingsPath);

    for (const auto& [name, lvl] : settings.logChannelLevels) {
        logger->setChannelLevel(name, lvl);
    }

    empire::audio::AudioSystem audio;
    if (!audio.init()) {
        GAME_LOG_INFO("Audio disabled");
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
        std::fprintf(stderr, "[Init] font not  loaded: running without text\n");
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

        static std::map<std::string, float, std::less<>> lastBusVolumes;
        if (lastBusVolumes != settings.busVolumes) {
            for (const auto& [name, v] : settings.busVolumes) {
                audio.setBusVolume(name, v);
            }
            lastBusVolumes = settings.busVolumes;
        }

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
    settings.logChannelLevels.clear();
    settings::save(settings, kSettingsPath);
    save.save(state, kSavePath);

    audio.shutdown();
    font.destroy();
    input.detach();
    renderer.shutdown();
    window.destroy();
    return 0;
}