#include "render/Window.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <cstdio>

namespace game::render {
Window::~Window() {
    destroy();
}

bool Window::create(int w, int h, const std::string& title) {
    if (m_handle) {
        return false;
    }

    if (!glfwInit()) {
        std::fprintf(stderr, "[Window] glfwInit failed\n");
        return false;
    }
    m_glfwOwned = true;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_handle = glfwCreateWindow(w, h, title.c_str(), nullptr, nullptr);
    if (!m_handle) {
        std::fprintf(stderr, "[Window]  glfwCreateWindow failed\n");
        glfwTerminate();
        m_glfwOwned = false;
        return false;
    }

    glfwMakeContextCurrent(m_handle);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::fprintf(stderr, "[Window] gladLoadGLLoader failed\n");
        glfwDestroyWindow(m_handle);
        m_handle = nullptr;
        glfwTerminate();
        m_glfwOwned = false;
        return false;
    }

    glfwSwapInterval(1);
    return true;
}

void Window::destroy() {
    if (m_handle) {
        glfwDestroyWindow(m_handle);
        m_handle = nullptr;
    }
    if (m_glfwOwned) {
        glfwTerminate();
        m_glfwOwned = false;
    }
}

bool Window::shouldClose() const {
    return m_handle ? glfwWindowShouldClose(m_handle) : true;
}

void Window::pollEvents() {
    glfwPollEvents();
}

void Window::swapBuffers() {
    if (m_handle) {
        glfwSwapBuffers(m_handle);
    }
}
void Window::requestClose() {
    if (m_handle) {
        glfwSetWindowShouldClose(m_handle, GLFW_TRUE);
    }
}
double Window::time() const {
    return glfwGetTime();
}
void Window::framebufferSize(int& w, int& h) const {
    w = 0;
    h = 0;
    if (m_handle) {
        glfwGetFramebufferSize(m_handle, &w, &h);
    }
}
void Window::applyVsync(bool enabled) {
    if (!m_handle) {
        return;
    }
    glfwMakeContextCurrent(m_handle);
    glfwSwapInterval(enabled ? 1 : 0);
}

bool Window::isFullscreen() const noexcept {
    if (!m_handle) {
        return false;
    }
    return glfwGetWindowMonitor(m_handle) != nullptr;
}

std::vector<std::string> Window::listMonitors() const {
    std::vector<std::string> out;
    int count = 0;
    GLFWmonitor** monitors = glfwGetMonitors(&count);
    for (int i = 0; i < count; ++i) {
        const char* name = glfwGetMonitorName(monitors[i]);
        out.push_back(name ? name : "<unnamed>");
    }
    if (out.empty()) {
        out.push_back("Default");
    }
    return out;
}
void Window::applyWindowMode(WindowMode mode, int w, int h, int monitorIndex) {
    if (!m_handle) {
        return;
    }

    int count = 0;
    GLFWmonitor** monitors = glfwGetMonitors(&count);
    if (count <= 0) {
        return;
    }
    if (monitorIndex < 0 || monitorIndex >= count) {
        monitorIndex = 0;
    }
    GLFWmonitor* target = monitors[monitorIndex];

    int mx = 0;
    int my = 0;
    glfwGetMonitorPos(target, &mx, &my);
    const GLFWvidmode* vid = glfwGetVideoMode(target);
    if (!vid) {
        return;
    }

    switch (mode) {
        case WindowMode::Windowed: {
            glfwSetWindowAttrib(m_handle, GLFW_DECORATED, GLFW_TRUE);
            const int px = mx + (vid->width - w) / 2;
            const int py = my + (vid->height - h) / 2;
            glfwSetWindowMonitor(m_handle, nullptr, px, py, w, h, 0);
            break;
        }
        case WindowMode::Borderless: {
            glfwSetWindowAttrib(m_handle, GLFW_DECORATED, GLFW_FALSE);
            glfwSetWindowMonitor(m_handle, nullptr, mx, my, vid->width,
                                 vid->height, 0);
            break;
        }
        case WindowMode::Fullscreen: {
            glfwSetWindowAttrib(m_handle, GLFW_DECORATED, GLFW_TRUE);
            glfwSetWindowMonitor(m_handle, target, mx, my, vid->width,
                                 vid->height, vid->refreshRate);
            break;
        }
    }
    m_windowMode = mode;
}

std::string Window::gpuName() const {
    const GLubyte* s = glGetString(GL_RENDERER);
    return s ? reinterpret_cast<const char*>(s) : "<unknown>";
}

std::string Window::glVersion() const {
    const GLubyte* s = glGetString(GL_VERSION);
    return s ? reinterpret_cast<const char*>(s) : "<unknown>";
}
}  // namespace game::render