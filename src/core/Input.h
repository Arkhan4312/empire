#pragma once
#include <glm/glm.hpp>

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
}  // namespace game