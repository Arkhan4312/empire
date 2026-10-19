#pragma once
namespace empire::audio {
class AudioSystem;
} // namespace empire::audio
namespace game {
class GameLogic;
class GameState;
class InputSystem;
class SaveSystem;
class SceneManager;
struct Settings;
namespace render {
class Window;
class Renderer;
class Font;
}  // namespace render
namespace ui {
struct UIContext;
} // namespace ui

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
    empire::audio::AudioSystem& audio;

    render::Font* font = nullptr;
    const char* savePath = "save.json";
};
}  // namespace game