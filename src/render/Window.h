#pragma once
#include <string>
#include <vector>

#include "settings/WindowMode.h"
struct GLFWwindow;
namespace game::render {

class Window {
public:
    Window() = default;
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool create(int w, int h, const std::string& title);
    void destroy();

    bool shouldClose() const;
    void pollEvents();
    void swapBuffers();

    void requestClose();

    double time() const;
    void framebufferSize(int& w, int& h) const;
    GLFWwindow* handle() const noexcept {
        return m_handle;
    }

    void applyVsync(bool enabled);

    bool isFullscreen() const noexcept;

    std::vector<std::string> listMonitors() const;
    void applyWindowMode(WindowMode mode, int w, int h, int monitorIndex);
    WindowMode windowMode() const noexcept {
        return m_windowMode;
    }

    std::string gpuName() const;
    std::string glVersion() const;
private:
    WindowMode m_windowMode = WindowMode::Windowed;
    GLFWwindow* m_handle = nullptr;
    bool m_glfwOwned = false;
};
}  // namespace game::render