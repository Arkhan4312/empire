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
    const float vW = kBaseVirtualW / m_uiScale;
    const float vH = kBaseVirtualH / m_uiScale;

    const float sx = static_cast<float>(w) / vW;
    const float sy = static_cast<float>(h) / vH;
    const float scale = std::min(sx, sy);

    const int vpW = static_cast<int>(vW * scale);
    const int vpH = static_cast<int>(vH * scale);
    const int vpX = (w - vpW) / 2;
    const int vpY = (h - vpH) / 2;

    glViewport(0, 0, w, h);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glViewport(vpX, vpY, vpW, vpH);
    m_virtualSize = {vW, vH};
    m_screenSize = {vW, vH};
    m_scale = scale;
    m_viewport = {static_cast<float>(vpX), static_cast<float>(vpY),
                  static_cast<float>(vpW), static_cast<float>(vpH)};
    glClearColor(m_clearColor.r * m_brightness, m_clearColor.g * m_brightness,
                 m_clearColor.b * m_brightness, m_clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT);

    const glm::mat4 proj = glm::ortho(0.0f, vW, vH, 0.0f, -1.0f, 1.0f);
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
    std::size_t i = 0;
    while (i < text.size()) {
        const std::uint32_t cp = utf8Decode(text, i);
        if (cp == '\n') {
            penX = pos.x;
            penY = lineH;
            continue;
        }
        if (cp == '\r') {
            continue;
        }
        const Glyph* g = font.glyph(cp);
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

void Renderer::setUIScale(float s) {
    if (s < 0.75f) {
        s = 0.75f;
    }
    if (s > 2.0f) {
        s = 2.0f;
    }
    m_uiScale = s;
}

glm::vec2 Renderer::realToVirtual(const glm::vec2& real) const noexcept {
    if (m_scale <= 0.0f) {
        return real;
    }
    return glm::vec2((real.x - m_viewport.x) / m_scale,
                     (real.y - m_viewport.y) / m_scale);
}

void Renderer::setScissorVirtual(const glm::vec2& pos, const glm::vec2& size) {
    int winW = 0;
    int winH = 0;
    if (m_window) {
        m_window->framebufferSize(winW, winH);
    }
    if (winW <= 0 || winH <= 0) {
        return;
    }
    const float realX = m_viewport.x + pos.x * m_scale;
    const float realY = m_viewport.y + pos.y * m_scale;
    const float realW = size.x * m_scale;
    const float realH = size.y * m_scale;

    const int glY = winH - static_cast<int>(std::round(realY + realH));

    glEnable(GL_SCISSOR_TEST);
    glScissor(static_cast<int>(std::round(realX)), glY,
              static_cast<int>(std::round(realW)),
              static_cast<int>(std::round(realH)));
}

void Renderer::clearScissor() {
    glDisable(GL_SCISSOR_TEST);
}

void Renderer::setBrightness(float b) noexcept {
    if (b < 0.5f) {
        b = 0.5f;
    }
    if (b > 1.5f) {
        b = 1.5f;
    }
    m_brightness = b;
    m_batch.setBrightness(b);
}

}  // namespace game::render