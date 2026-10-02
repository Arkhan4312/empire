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
                          reinterpret_cast<void*>(0));
    // location = 1 : vec2 uv
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void*>(sizeof(float) * 2));
    // location = 2 : vec4  color
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void*>(sizeof(float) * 4));

    glBindVertexArray(0);

    m_vertices.reserve(m_maxQuads * 4);
    m_indices.reserve(m_maxQuads * 6);

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
    m_initialized = false;
}

void SpriteBatch::begin(const glm::mat4& viewProj) {
    m_viewProj = viewProj;
    m_vertices.clear();
    m_indices.clear();
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

    if (m_vertices.size() + 4 > m_maxQuads * 4) {
        flush();
        m_currentTexture = tex.id();
    }

    pushQuad(tex, pos, size, uvRect, color, rotationRad);
}

void SpriteBatch::flush() {
    if (m_vertices.empty() || m_currentTexture == 0) {
        m_vertices.clear();
        m_indices.clear();
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

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(m_indices.size() * sizeof(std::uint32_t)),
        m_indices.data(), GL_DYNAMIC_DRAW);

    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indices.size()),
                   GL_UNSIGNED_INT, nullptr);

    glBindVertexArray(0);
    m_shader.unbind();

    m_vertices.clear();
    m_indices.clear();
}

void SpriteBatch::pushQuad(const Texture&, const glm::vec2& pos,
                           const glm::vec2& size, const glm::vec4& uvRect,
                           const glm::vec4& color, float rotationRad) {
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

    const auto base = static_cast<std::uint32_t>(m_vertices.size());

    m_vertices.push_back({p0, {u0, v0}, color});
    m_vertices.push_back({p1, {u1, v0}, color});
    m_vertices.push_back({p2, {u1, v1}, color});
    m_vertices.push_back({p3, {u0, v1}, color});

    m_indices.push_back(base + 0);
    m_indices.push_back(base + 1);
    m_indices.push_back(base + 2);
    m_indices.push_back(base + 2);
    m_indices.push_back(base + 3);
    m_indices.push_back(base + 0);
}

}  // namespace game::render