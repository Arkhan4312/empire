#pragma once
#include <string>

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

private:
    GLFWwindow* m_handle = nullptr;
    bool m_glfwOwned = false;
};
}  // namespace game::render