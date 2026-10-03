#pragma once
#include <glm/glm.hpp>

#include "core/Input.h"
#include "render/Renderer.h"
namespace game::render {
class Renderer;
class Font;
}  // namespace game::render

namespace game::ui {

class Widget;
// Colors and metrics used for UI rendering
struct UITheme {
    glm::vec4 panelBg{0.10f, 0.11f, 0.14f, 1.0f};
    glm::vec4 panelBorder{0.25f, 0.27f, 0.32f, 1.0f};
    glm::vec4 buttonBg{0.20f, 0.24f, 0.30f, 1.0f};
    glm::vec4 buttonHover{0.28f, 0.34f, 0.42f, 1.0f};
    glm::vec4 buttonPressed{0.14f, 0.17f, 0.22f, 1.0f};
    glm::vec4 buttonDisabled{0.16f, 0.16f, 0.18f, 1.0f};
    glm::vec4 textColor{0.95f, 0.95f, 0.95f, 1.0f};
    glm::vec4 textDisabled{0.55f, 0.55f, 0.55f, 1.0f};

    float padding = 8.0f;
    float spacing = 6.0f;
    float borderWidth = 1.0f;
};

// UI context: renderer, font, input, theme, and interaction state.
struct UIContext {
    render::Renderer* renderer = nullptr;
    render::Font* font = nullptr;
    const game::InputState* input = nullptr;
    UITheme theme;

    Widget* hovered = nullptr;
    Widget* active = nullptr;
    Widget* focused = nullptr;

    void beginFrame();
    void endFrame();

    glm::vec2 mousePos() const {
        return input ? input->mousePos : glm::vec2(0.0f);
    }
    bool mousePressed(int b = mouse::Left) const {
        return input && input->isMousePressed(b);
    }
    bool mouseDown(int b = mouse::Left) const {
        return input && input->isMouseDown(b);
    }
    bool mouseReleased(int b = mouse::Left) const {
        return input && input->isMouseReleased(b);
    }

    void setFocus(Widget* w) noexcept {
        focused = w;
    }
    void clearFocus(Widget* w) noexcept {
        if (focused == w) {
            focused = nullptr;
        }
    }

    glm::vec2 screenSize() const {
        return renderer ? renderer->screenSize() : glm::vec2(0.0f);
    }
};

}  // namespace game::ui