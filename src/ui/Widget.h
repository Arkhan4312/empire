#pragma once
#include <glm/glm.hpp>
#include <memory>

namespace game::ui {
class UIContext;
// Base class for a UI widget
class Widget {
public:
    virtual ~Widget() = default;
    // measures desired size
    virtual glm::vec2 measure(UIContext& ctx, const glm::vec2& available) = 0;
    // sets position and size
    virtual void arrange(UIContext& ctx, const glm::vec2& pos,
                         const glm::vec2& size) {
        (void)ctx;
        m_pos = pos;
        m_size = size;
    }
    // update logic
    virtual void update(UIContext& ctx) {
        (void)ctx;
    }
    // rendering
    virtual void render(UIContext& ctx) = 0;
    bool hitTest(const glm::vec2& p) const noexcept {
        return p.x >= m_pos.x && p.y >= m_pos.y && p.x < m_pos.x + m_size.x &&
               p.y < m_pos.y + m_size.y;
    }

    const glm::vec2& pos() const noexcept {
        return m_pos;
    }
    const glm::vec2& size() const noexcept {
        return m_size;
    }
    bool visible = true;
    bool enabled = true;

protected:
    glm::vec2 m_pos{0.0f, 0.0f};
    glm::vec2 m_size{0.0f, 0.0f};
};

using WidgetPtr = std::unique_ptr<Widget>;
}  // namespace game::ui