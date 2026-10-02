#pragma once
#include <cstddef>
#include <cstdint>
#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "render/Shader.h"
#include "render/Texture.h"

namespace game::render {

struct Vertex2D {
    glm::vec2 pos;
    glm::vec2 uv;
    glm::vec4 color;
};
// Butcher for 2D sprites/tris. Collect vertices and draw with one
// glDrawElements
class SpriteBatch {
public:
    SpriteBatch() = default;
    ~SpriteBatch();

    SpriteBatch(const SpriteBatch&) = delete;
    SpriteBatch& operator=(const SpriteBatch&) = delete;

    bool init(const std::string& vertPath, const std::string& fragPath);
    void shutdown();

    void begin(const glm::mat4& viewProj);
    void end();

    void draw(const Texture& tex, const glm::vec2& pos, const glm::vec2& size,
              const glm::vec4& color = glm::vec4(1.0f),
              float rotationRad = 0.0f);

    void drawUV(const Texture& tex, const glm::vec2& pos, const glm::vec2& size,
                const glm::vec4& uvRect,
                const glm::vec4& color = glm::vec4(1.0f),
                float rotationRad = 0.0f);

private:
    void flush();
    void pushQuad(const Texture& tex, const glm::vec2& pos,
                  const glm::vec2& size, const glm::vec4& uvRect,
                  const glm::vec4& color, float rotationRad);

    Shader m_shader;
    std::uint32_t m_vao = 0;
    std::uint32_t m_vbo = 0;
    std::uint32_t m_ibo = 0;

    std::vector<Vertex2D> m_vertices;
    std::vector<std::uint32_t> m_indices;

    glm::mat4 m_viewProj{1.0f};
    std::uint32_t m_currentTexture = 0;
    std::size_t m_maxQuads = 4096;
    bool m_begun = false;
    bool m_initialized = false;
};
}  // namespace game::render