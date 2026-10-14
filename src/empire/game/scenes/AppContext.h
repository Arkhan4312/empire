#pragma once

namespace game {

class GameLogic;
class GameState;
class InputSystem;
class SaveSystem;
class SceneManager;
struct Settings;
namespace audio {
class AudioSystem;
}

namespace render {
class Window;
class Renderer;
class Font;
}  // namespace render
namespace ui {
struct UIContext;
}

struct AppContext {
    GameState& state;
    GameLogic& logic;
    SaveSystem& save;
    Settings& settings;
    render::Window& window;
    render::Renderer& renderer;
    InputSystem& input;
    ui::UIContext& ui;
    SceneManager& scenes;
    audio::AudioSystem& audio;

    render::Font* font = nullptr;
    const char* savePath = "save.json";
};
}  // namespace game