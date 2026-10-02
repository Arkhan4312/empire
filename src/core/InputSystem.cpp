#include "core/InputSystem.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace game {
InputSystem::~InputSystem() {
    detach();
}
bool InputSystem::attach(GLFWwindow* window) {
    if (!window) {
        return false;
    }
    m_window = window;
    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, &InputSystem::keyCallback);
    glfwSetMouseButtonCallback(window, &InputSystem::mouseButtonCallback);
    glfwSetCursorPosCallback(window, &InputSystem::cursorPosCallback);
    glfwSetScrollCallback(window, &InputSystem::scrollCallback);
    return true;
}

void InputSystem::detach() {
    if (!m_window) {
        return;
    }

    glfwSetWindowUserPointer(m_window, nullptr);
    glfwSetKeyCallback(m_window, nullptr);
    glfwSetMouseButtonCallback(m_window, nullptr);
    glfwSetCursorPosCallback(m_window, nullptr);
    glfwSetScrollCallback(m_window, nullptr);
    m_window = nullptr;
    m_hasMousePos = false;
}

void InputSystem::beginFrame() {
    m_state.beginFrame();
}
void InputSystem::keyCallback(GLFWwindow* w, int key, int /*scancode*/,
                              int action, int /*mods*/) {
    auto* self = static_cast<InputSystem*>(glfwGetWindowUserPointer(w));
    if (!self || key < 0 || key >= InputState::kMaxKeys) {
        return;
    }

    if (action == GLFW_PRESS) {
        self->m_state.keyDown[key] = true;
        self->m_state.keyPressed[key] = true;
    } else if (action == GLFW_RELEASE) {
        self->m_state.keyDown[key] = false;
        self->m_state.keyReleased[key] = true;
    }
}

void InputSystem::mouseButtonCallback(GLFWwindow* w, int button, int action,
                                      int mods) {
    auto* self = static_cast<InputSystem*>(glfwGetWindowUserPointer(w));
    if (!self || button < 0 || button >= 3) {
        return;
    }

    if (action == GLFW_PRESS) {
        self->m_state.mouseDown[button] = true;
        self->m_state.mousePressed[button] = true;
    } else if (action == GLFW_RELEASE) {
        self->m_state.mouseDown[button] = false;
        self->m_state.mouseReleased[button] = true;
    }
}
void InputSystem::cursorPosCallback(GLFWwindow* w, double x, double y) {
    auto* self = static_cast<InputSystem*>(glfwGetWindowUserPointer(w));
    if (!self) {
        return;
    }

    const glm::vec2 pos{static_cast<float>(x), static_cast<float>(y)};
    if (self->m_hasMousePos) {
        self->m_state.mouseDelta += pos - self->m_state.mousePos;
    }
    self->m_state.mousePos = pos;
    self->m_hasMousePos = true;
}

void InputSystem::scrollCallback(GLFWwindow* w, double xoffset,
                                 double yoffset) {
    auto* self = static_cast<InputSystem*>(glfwGetWindowUserPointer(w));
    if (!self) {
        return;
    }
    self->m_state.scrollDelta.x += static_cast<float>(xoffset);
    self->m_state.scrollDelta.y += static_cast<float>(yoffset);
}
}  // namespace game