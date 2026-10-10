#include "render/SpriteBatch.h"

#include <glad/glad.h>

#include <cmath>
#include <glm/gtc/type_ptr.hpp>

namespace game::render {
SpriteBatch::~SpriteBatch() {
    shutdown();
}

bool SpriteBatch::init(const std::string& vertPath,
                       const std::string& fragPath) {
    if (m_initialized) {
        return true;
    }
    if (!m_shader.loadFromFiles(vertPath, fragPath)) {
        return false;
    }

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ibo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);

    const GLsizei stride = static_cast<GLsizei>(sizeof(Vertex2D));

    // location = 0 : vec2 pos
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void*>(offsetof(Vertex2D, pos)));
    // location = 1 : vec2 uv
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void*>(offsetof(Vertex2D, uv)));
    // location = 2 : vec4  color
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void*>(offsetof(Vertex2D, color)));

    {
        std::vector<std::uint32_t> idx(m_maxQuads * 6);
        for (std::size_t i = 0; i < m_maxQuads; ++i) {
            const std::uint32_t b = static_cast<std::uint32_t>(i) * 4u;
            idx[i * 6 + 0] = b + 0;
            idx[i * 6 + 1] = b + 1;
            idx[i * 6 + 2] = b + 2;
            idx[i * 6 + 3] = b + 2;
            idx[i * 6 + 4] = b + 3;
            idx[i * 6 + 5] = b + 0;
        }
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(idx.size() * sizeof(std::uint32_t)),
            idx.data(), GL_STATIC_DRAW);
    }

    glBindVertexArray(0);

    m_vertices.reserve(m_maxQuads * 4);
    m_initialized = true;
    return true;
}

void SpriteBatch::shutdown() {
    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    if (m_vbo) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    if (m_ibo) {
        glDeleteBuffers(1, &m_ibo);
        m_ibo = 0;
    }
    m_shader.destroy();
    m_vertices.clear();
    m_begun = false;
    m_currentTexture = 0;
    m_initialized = false;
}

void SpriteBatch::begin(const glm::mat4& viewProj) {
    m_viewProj = viewProj;
    m_vertices.clear();
    m_currentTexture = 0;
    m_begun = true;
}

void SpriteBatch::end() {
    flush();
    m_begun = false;
}

void SpriteBatch::draw(const Texture& tex, const glm::vec2& pos,
                       const glm::vec2& size, const glm::vec4& color,
                       float rotationRad) {
    drawUV(tex, pos, size, {0.0f, 0.0f, 1.0f, 1.0f}, color, rotationRad);
}

void SpriteBatch::drawUV(const Texture& tex, const glm::vec2& pos,
                         const glm::vec2& size, const glm::vec4& uvRect,
                         const glm::vec4& color, float rotationRad) {
    if (!m_begun) {
        return;
    }
    if (!m_vertices.empty() && m_currentTexture != tex.id()) {
        flush();
    }
    if (m_vertices.empty()) {
        m_currentTexture = tex.id();
    }

    if (m_vertices.size() + 4 > m_maxQuads * 6) {
        flush();
        m_currentTexture = tex.id();
    }

    pushQuad(tex, pos, size, uvRect, color, rotationRad);
}

void SpriteBatch::flush() {
    if (m_vertices.empty() || m_currentTexture == 0) {
        m_vertices.clear();
        return;
    }

    m_shader.bind();
    m_shader.setMat4("uViewProj", m_viewProj);
    m_shader.setInt("uTexture", 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_currentTexture);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(m_vertices.size() * sizeof(Vertex2D)),
                 m_vertices.data(), GL_DYNAMIC_DRAW);

    const std::size_t quadCount = m_vertices.size() / 4;
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(quadCount * 6),
                   GL_UNSIGNED_INT, nullptr);

    glBindVertexArray(0);
    m_shader.unbind();

    m_vertices.clear();
}

void SpriteBatch::pushQuad(const Texture&, const glm::vec2& pos,
                           const glm::vec2& size, const glm::vec4& uvRect,
                           const glm::vec4& color, float rotationRad) {
    glm::vec4 tint = color;
    tint.r = std::min(1.0f, tint.r * m_brightness);
    tint.g = std::min(1.0f, tint.g * m_brightness);
    tint.b = std::min(1.0f, tint.b * m_brightness);

    const float c = std::cos(rotationRad);
    const float s = std::sin(rotationRad);
    const glm::vec2 half = size * 0.5f;
    const glm::vec2 center = pos + half;

    auto rot = [&](const glm::vec2& p) -> glm::vec2 {
        return glm::vec2(p.x * c - p.y * s, p.x * s + p.y * c) + center;
    };

    const glm::vec2 p0 = rot({-half.x, -half.y});
    const glm::vec2 p1 = rot({half.x, -half.y});
    const glm::vec2 p2 = rot({half.x, half.y});
    const glm::vec2 p3 = rot({-half.x, half.y});

    const float u0 = uvRect.x;
    const float v0 = uvRect.y;
    const float u1 = uvRect.z;
    const float v1 = uvRect.w;

    m_vertices.push_back({p0, {u0, v0}, tint});
    m_vertices.push_back({p1, {u1, v0}, tint});
    m_vertices.push_back({p2, {u1, v1}, tint});
    m_vertices.push_back({p3, {u0, v1}, tint});
}

}  // namespace game::render