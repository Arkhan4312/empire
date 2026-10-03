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
}  // namespace mouse
struct InputState {
    static constexpr int kMaxKeys = 512;

    glm::vec2 mousePos{0.0f, 0.0f};
    glm::vec2 mouseDelta{0.0f, 0.0f};
    glm::vec2 scrollDelta{0.0f, 0.0f};

    bool mouseDown[3] = {false, false, false};
    bool mousePressed[3] = {false, false, false};
    bool mouseReleased[3] = {false, false, false};

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
        return b >= 0 && b < 3 && mouseDown[b];
    }
    bool isMousePressed(int b) const noexcept {
        return b >= 0 && b < 3 && mousePressed[b];
    }
    bool isMouseReleased(int b) const noexcept {
        return b >= 0 && b < 3 && mouseReleased[b];
    }

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
        case 32:
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
        case 256:
            return "Escape";
        case 257:
            return "Enter";
        case 258:
            return "Tab";
        case 259:
            return "Backspace";
        case 260:
            return "Insert";
        case 261:
            return "Delete";
        case 262:
            return "Right";
        case 263:
            return "Left";
        case 264:
            return "Down";
        case 265:
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