#pragma once
#include <cstdint>
#include <string>

namespace game::render {

// stb_image will be added soon. stub version
class Texture {
public:
    Texture() = default;
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    bool loadFromFile(const std::string& path);

    bool createSolid(int w, int h, std::uint32_t rgba = 0xFFFFFFFFu);
    bool createRGBA(int w, int h, const unsigned char* pixels);
    void destroy();

    void bind(std::uint32_t unit = 0) const;
    void unbind() const;

    int width() const noexcept {
        return m_width;
    }

    int height() const noexcept {
        return m_height;
    }

    std::uint32_t id() const noexcept {
        return m_id;
    }

    bool valid() const noexcept {
        return m_id != 0;
    }

private:
    bool uploadRGBA(int w, int h, const unsigned char* pixels);
    std::uint32_t m_id = 0;
    int m_width = 0;
    int m_height = 0;
};

}  // namespace game::render