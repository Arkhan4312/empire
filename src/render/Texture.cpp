#include "render/Texture.h"

#include <glad/glad.h>
#include <stb_image.h>

#include <cstdio>
#include <vector>
namespace game::render {
Texture::~Texture() {
    destroy();
}

bool Texture::loadFromFile(const std::string& path) {
    stbi_set_flip_vertically_on_load(0);
    int w = 0;
    int h = 0;
    int channels = 0;
    unsigned char* pixels = stbi_load(path.c_str(), &w, &h, &channels, 4);
    if (!pixels) {
        std::fprintf(stderr, "[Texture] stbi_load failed for '%s': %s\n",
                     path.c_str(), stbi_failure_reason);
        return false;
    }
    const bool ok = uploadRGBA(w, h, pixels);
    stbi_image_free(pixels);
    return ok;
}

bool Texture::createSolid(int w, int h, std::uint32_t rgba) {
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

    return uploadRGBA(w, h, pixels.data());
}

bool Texture::createRGBA(int w, int h, const unsigned char* pixels) {
    if (!pixels || w <= 0 || h <= 0) {
        return false;
    }
    return uploadRGBA(w, h, pixels);
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
bool Texture::uploadRGBA(int w, int h, const unsigned char* pixels) {
    destroy();

    GLuint id = 0;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 pixels);
    glBindTexture(GL_TEXTURE_2D, 0);

    m_id = id;
    m_width = w;
    m_height = h;
    return true;
}
}  // namespace game::render