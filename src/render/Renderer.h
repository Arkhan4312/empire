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
    bool init(Window& window);
    void shutdown();

    void beginFrame();
    void endFrame();

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

private:
    Window* m_window = nullptr;
    SpriteBatch m_batch;
    Texture m_white;
    glm::vec4 m_clearColor{0.06f, 0.07f, 0.10f, 1.0f};
};
}  // namespace game::render