#include <chrono>
#include <cstdint>
#include <cstdio>

#include "content/Content.h"
#include "core/GameLogic.h"
#include "core/GameState.h"
#include "render/Renderer.h"
#include "render/Window.h"
#include "systems/SaveSystem.h"
#include "core/Input.h"
#include "core/InputSystem.h"
#include "render/Font.h"

using namespace game;

static std::int64_t nowSeconds() {
    using namespace std::chrono;
    return duration_cast<seconds>(system_clock::now().time_since_epoch())
        .count();
}

int main() {
    const char* kSavePath = "save.json";

    GameState state;
    content::initNewGame(state);

    GameLogic logic;
    SaveSystem save;

    if (save.load(state, kSavePath)) {
        const auto away =
            SaveSystem::computeOfflineSeconds(state, nowSeconds());
        if (away > 0) {
            const double sim = SaveSystem::applyOffline(state, logic, away);
            std::printf("[Offline] away %llds, simulated %.1fs\n",
                        (long long)away, sim);
        }
    } else {
        std::printf("[Init] new game\n");
    }

    render::Window window;
    if (!window.create(1280, 720, "Empire from Nothing - Core + Gl stub")) {
        std::fprintf(stderr, "Window creation failed\n");
        return 1;
    }

    InputSystem input;
    if (!input.attach(window.handle())) {
        std::fprintf(stderr, "InputSystem attach failed\n");
        window.destroy();
        return 1;
    }
    render::Renderer renderer;
    if (!renderer.init(window)) {
        std::fprintf(stderr, "Renderer init failed\n");
        window.destroy();
        return 1;
    }

    render::Font font;
    const bool fontOk = font.loadFromFile(std::string(EMPIRE_ASSETS_DIR) + "/fonts/main.ttf", 28.0f);
    if (!fontOk) {
        std::fprintf(stderr, "[Init] font not loaded, running without text\n");
    }
  
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

        TickContext ctx{dt};
        logic.tick(state, ctx);

        renderer.beginFrame();
        renderer.drawRect({ 40.0f,40.0f }, { 260.0f,90.0f }, { 0.15f,0.35f,0.65f,1.0f });
        renderer.drawRect({ 60.0f,60.0f }, { 220.0f,50.0f }, { 0.95f,0.65f,0.15f,1.0f });
        renderer.drawRect({ 340.0f,40.0f }, { 200.0f,90.0f }, { 0.25f,0.65f,0.35f,1.0f });
        if (fontOk) {
            char buf[256];
            std::snprintf(buf, sizeof(buf), "plastic: %.1f", state.resources.get(ResourceType::PLASTIC));
            renderer.drawText(font, buf, { 60.0f, 40.0f }, { 1.0f,1.0f,1.0f,1.0f });
        }
        renderer.endFrame();

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