#pragma once
#include "core/Input.h"

struct GLFWwindow;

namespace game {
class InputSystem {
public:
    InputSystem() = default;
    ~InputSystem();

    InputSystem(const InputSystem&) = delete;
    InputSystem& operator=(const InputSystem&) = delete;

    bool attach(GLFWwindow* window);
    void detach();
    void beginFrame();

    const InputState& state() const noexcept {
        return m_state;
    }
    InputState& state() noexcept {
        return m_state;
    }

private:
    static void keyCallback(GLFWwindow*, int key, int scancode, int action,
                            int mods);
    static void mouseButtonCallback(GLFWwindow*, int button, int action,
                                    int mods);
    static void cursorPosCallback(GLFWwindow*, double x, double y);
    static void scrollCallback(GLFWwindow*, double xoffset, double yoffset);

    GLFWwindow* m_window = nullptr;
    InputState m_state;
    bool m_hasMousePos = false;
};
}  // namespace game