#include "render/Texture.h"

#include <glad/glad.h>

#include <cstdio>
#include <vector>

namespace game::render {
Texture::~Texture() {
    destroy();
}

bool Texture::loadFromFile(const std::string& path) {
    // int w, h, ch;
    // stbi_set_flip_vertically_on_load(1);
    // unsigned char* px = stbi_loaad(path.c_str(), &w, &h, &ch, 4);
    // ............................................................
    std::fprintf(stderr, "[Texture] (stub) not loading '%s', using 1x1 white\n",
                 path.c_str());
    return createSolid(1, 1, 0xFFFFFFFFu);
}

bool Texture::createSolid(int w, int h, std::uint32_t rgba) {
    destroy();
    if (w <= 0 || h <= 0) {
        return false;
    }

    const std::uint8_t r = static_cast<std::uint8_t>((rgba >> 24) & 0xFF);
    const std::uint8_t g = static_cast<std::uint8_t>((rgba >> 16) & 0xFF);
    const std::uint8_t b = static_cast<std::uint8_t>((rgba >> 8) & 0xFF);
    const std::uint8_t a = static_cast<std::uint8_t>(rgba & 0xFF);

    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(w) *
                                     static_cast<std::size_t>(h) * 4u);
    for (std::size_t i = 0; i < pixels.size(); i += 4) {
        pixels[i + 0] = r;
        pixels[i + 1] = g;
        pixels[i + 2] = b;
        pixels[i + 3] = a;
    }

    GLuint id = 0;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    m_id = id;
    m_width = w;
    m_height = h;
    return true;
}

void Texture::destroy() {
    if (m_id) {
        GLuint id = m_id;
        glDeleteTextures(1, &id);
        m_id = 0;
        m_width = 0;
        m_height = 0;
    }
}

void Texture::bind(std::uint32_t unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, m_id);
}

void Texture::unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
}
}  // namespace game::render