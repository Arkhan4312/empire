#include "render/Renderer.h"

#include <glad/glad.h>

#include <cstdio>
#include <glm/gtc/matrix_transform.hpp>

#include "render/Window.h"
#ifndef EMPIRE_ASSETS_DIR
#define EMPIRE_ASSETS_DIR "assets"
#endif

namespace game::render {
bool Renderer::init(Window& window) {
    m_window = &window;

    const std::string vertPath =
        std::string(EMPIRE_ASSETS_DIR) + "/shaders/sprite.vert";
    const std::string fragPath =
        std::string(EMPIRE_ASSETS_DIR) + "/shaders/sprite.frag";

    if (!m_batch.init(vertPath, fragPath)) {
        std::fprintf(stderr, "[Renderer] SpriteBatch init failed\n");
        return false;
    }
    if (!m_white.createSolid(1, 1, 0xFFFFFFFFu)) {
        std::fprintf(stderr, "[Renderer] white  texture creation  failed\n");
        return false;
    }

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    return true;
}

void Renderer::shutdown() {
    m_batch.shutdown();
    m_white.destroy();
    m_window = nullptr;
}

void Renderer::beginFrame() {
    int w = 0;
    int h = 0;
    if (m_window) {
        m_window->framebufferSize(w, h);
    }
    if (w <= 0 || h <= 0) {
        return;
    }

    glViewport(0, 0, w, h);
    m_screenSize = {static_cast<float>(w), static_cast<float>(h)};
    glClearColor(m_clearColor.r, m_clearColor.g, m_clearColor.b,
                 m_clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT);

    const glm::mat4 proj = glm::ortho(0.0f, static_cast<float>(w),
                                      static_cast<float>(h), 0.0f, -1.0f, 1.0f);
    m_batch.begin(proj);
}

void Renderer::endFrame() {
    m_batch.end();
}

void Renderer::drawRect(const glm::vec2& pos, const glm::vec2& size,
                        const glm::vec4& color) {
    m_batch.draw(m_white, pos, size, color);
}

void Renderer::drawSprite(const Texture& tex, const glm::vec2& pos,
                          const glm::vec2& size, const glm::vec4& color,
                          float rotationRad) {
    m_batch.draw(tex, pos, size, color, rotationRad);
}
void Renderer::drawText(const Font& font, std::string_view text,
                        const glm::vec2& pos, const glm::vec4& color,
                        float scale) {
    if (!font.valid()) {
        return;
    }
    const Texture& atlas = font.atlas();
    const float natural = font.ascent() - font.descent();
    const float lineH =
        (font.lineHeight() > natural ? font.lineHeight() : natural) * scale;

    float penX = pos.x;
    float penY = pos.y + font.ascent() * scale;

    for (char ch : text) {
        if (ch == '\n') {
            penX = pos.x;
            penY += lineH;
            continue;
        }
        if (ch == '\r') {
            continue;
        }

        const Glyph* g = font.glyph(ch);
        if (!g) {
            continue;
        }

        if (g->width > 0 && g->height > 0) {
            const glm::vec2 gpos = {penX + g->xoff * scale,
                                    penY + g->yoff * scale};
            const glm::vec2 gsize = {g->width * scale, g->height * scale};
            const glm::vec4 uv = {g->u0, g->v0, g->u1, g->v1};
            m_batch.drawUV(atlas, gpos, gsize, uv, color, 0.0f);
        }
        penX += g->xadvance * scale;
    }
}

}  // namespace game::render