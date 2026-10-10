#pragma once
#include <glm/glm.hpp>
#include <string_view>

#include "render/Font.h"
#include "render/SpriteBatch.h"
#include "render/Texture.h"

namespace game::render {
class Window;  // fwd
class Renderer {
public:
    static constexpr float kBaseVirtualW = 1280.0f;
    static constexpr float kBaseVirtualH = 720.0f;
    bool init(Window& window);
    void shutdown();

    void beginFrame();
    void endFrame();
    glm::vec2 screenSize() const noexcept {
        return m_screenSize;
    }
    void drawRect(const glm::vec2& pos, const glm::vec2& size,
                  const glm::vec4& color);

    void drawSprite(const Texture& tex, const glm::vec2& pos,
                    const glm::vec2& size,
                    const glm::vec4& color = glm::vec4(1.0f),
                    float rotationRad = 0.0f);
    void drawText(const Font& font, std::string_view text, const glm::vec2& pos,
                  const glm::vec4& color = glm::vec4(1.0f), float scale = 1.0f);

    const Texture& whiteTexture() const noexcept {
        return m_white;
    }

    SpriteBatch& batch() noexcept {
        return m_batch;
    }

    glm::vec2 virtualSize() const noexcept {
        return m_virtualSize;
    }

    void setUIScale(float s);
    float uiScale() const noexcept {
        return m_uiScale;
    }

    glm::vec2 realToVirtual(const glm::vec2& real) const noexcept;
    void setScissorVirtual(const glm::vec2& pos, const glm::vec2& size);
    void clearScissor();

    void setBrightness(float b) noexcept;
    float brightness() const noexcept {
        return m_brightness;
    }

private:
    Window* m_window = nullptr;
    SpriteBatch m_batch;
    Texture m_white;
    float m_brightness = 1.0f;
    glm::vec4 m_clearColor{0.06f, 0.07f, 0.10f, 1.0f};
    glm::vec2 m_screenSize{0.0f};
    float m_uiScale = 1.0f;
    glm::vec2 m_virtualSize{1280.0f, 720.0f};
    float m_scale = 1.0f;
    glm::vec4 m_viewport{0.0f, 0.0f, 1280.0f, 720.0f};
};

}  // namespace game::render