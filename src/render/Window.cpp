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
}  // namespace game::render