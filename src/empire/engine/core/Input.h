#pragma once
#include <cstdio>
#include <glm/glm.hpp>
#include <string>
namespace game {
namespace keys {
inline constexpr int Space = 32;
inline constexpr int A = 65;
inline constexpr int D = 68;
inline constexpr int E = 69;
inline constexpr int S = 83;
inline constexpr int U = 85;
inline constexpr int W = 87;
inline constexpr int Escape = 256;
inline constexpr int Enter = 257;
inline constexpr int Tab = 258;
inline constexpr int Backspace = 259;
inline constexpr int Right = 262;
inline constexpr int Left = 263;
inline constexpr int Down = 264;
inline constexpr int Up = 265;
}  // namespace keys
namespace mouse {
inline constexpr int Left = 0;
inline constexpr int Right = 1;
inline constexpr int Middle = 2;
inline constexpr int Button4 = 3;
inline constexpr int Button5 = 4;
inline constexpr int Button6 = 5;
inline constexpr int Button7 = 6;
inline constexpr int Button8 = 7;
}  // namespace mouse
struct InputState {
    static constexpr int kMaxKeys = 512;
    static constexpr int kMaxMouseButton = 8;

    glm::vec2 mousePos{0.0f, 0.0f};
    glm::vec2 mouseDelta{0.0f, 0.0f};
    glm::vec2 scrollDelta{0.0f, 0.0f};

    bool mouseDown[kMaxMouseButton] = {};
    bool mousePressed[kMaxMouseButton] = {};
    bool mouseReleased[kMaxMouseButton] = {};

    bool keyDown[kMaxKeys] = {};
    bool keyPressed[kMaxKeys] = {};
    bool keyReleased[kMaxKeys] = {};

    bool isKeyDown(int key) const noexcept {
        return key >= 0 && key < kMaxKeys && keyDown[key];
    }
    bool isKeyPressed(int key) const noexcept {
        return key >= 0 && key < kMaxKeys && keyPressed[key];
    }
    bool isKeyReleased(int key) const noexcept {
        return key >= 0 && key < kMaxKeys && keyReleased[key];
    }

    bool isMouseDown(int b) const noexcept {
        return b >= 0 && b < kMaxMouseButton && mouseDown[b];
    }
    bool isMousePressed(int b) const noexcept {
        return b >= 0 && b < kMaxMouseButton && mousePressed[b];
    }
    bool isMouseReleased(int b) const noexcept {
        return b >= 0 && b < kMaxMouseButton && mouseReleased[b];
    }
    // WARNING: call before glfwPollEvents();
    void beginFrame() noexcept {
        for (auto& b : mousePressed) {
            b = false;
        }

        for (auto& b : mouseReleased) {
            b = false;
        }

        for (auto& b : keyPressed) {
            b = false;
        }

        for (auto& b : keyReleased) {
            b = false;
        }
        mouseDelta = glm::vec2(0.0f);
        scrollDelta = glm::vec2(0.0f);
    }
};

inline std::string keyName(int key) {
    switch (key) {
        case keys::Space:
            return "Space";
        case 39:
            return "'";
        case 44:
            return ",";
        case 45:
            return "-";
        case 46:
            return ".";
        case 47:
            return "/";
        case 59:
            return ";";
        case 61:
            return "=";
        case 91:
            return "[";
        case 92:
            return "\\";
        case 93:
            return "]";
        case 96:
            return "`";
        case keys::Escape:
            return "Escape";
        case keys::Enter:
            return "Enter";
        case keys::Tab:
            return "Tab";
        case keys::Backspace:
            return "Backspace";
        case 260:
            return "Insert";
        case 261:
            return "Delete";
        case keys::Right:
            return "Right";
        case keys::Left:
            return "Left";
        case keys::Down:
            return "Down";
        case keys::Up:
            return "Up";
        case 266:
            return "PageUp";
        case 267:
            return "PageDown";
        case 268:
            return "Home";
        case 269:
            return "End";
        default:
            break;
    }
    if (key >= 48 && key <= 57) return std::string(1, static_cast<char>(key));
    if (key >= 65 && key <= 90) return std::string(1, static_cast<char>(key));
    if (key >= 290 && key <= 301) {
        char buf[8];
        std::snprintf(buf, sizeof(buf), "F%d", key - 289);
        return buf;
    }
    return "?";
}

}  // namespace game