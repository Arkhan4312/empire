#pragma once
#include <string>
#include <string_view>

#include "render/Texture.h"

namespace game::render {

struct Glyph {
    // UV
    float u0 = 0.0f;
    float v0 = 0.0f;
    float u1 = 0.0f;
    float v1 = 0.0f;
    // bitmap
    int width = 0;
    int height = 0;
    // offset
    int xoff = 0;
    int yoff = 0;
    float xadvance = 0.0f;
};

class Font {
public:
    Font() = default;
    ~Font() = default;

    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;

    bool loadFromFile(const std::string& path, float pixelSize = 32.0f);

    void destroy();

    bool valid() const noexcept {
        return m_loaded;
    }
    const Texture& atlas() const noexcept {
        return m_atlas;
    }
    float pixelSize() const noexcept {
        return m_pixelSize;
    }
    float ascent() const noexcept {
        return m_ascent;
    }
    float descent() const noexcept {
        return m_descent;
    }
    float lineHeight() const noexcept {
        return m_lineHeight;
    }

    const Glyph* glyph(char c) const noexcept;

    float measureText(std::string_view text) const noexcept;

private:
    static constexpr int kFirstChar = 32;
    static constexpr int kLastChar = 126;
    static constexpr int kGlyphCount = kLastChar - kFirstChar + 1;

    Texture m_atlas;
    Glyph m_glyphs[kGlyphCount]{};
    float m_pixelSize = 0.0f;
    float m_ascent = 0.0f;
    float m_descent = 0.0f;
    float m_lineHeight = 0.0f;
    bool m_loaded = false;
};
}  // namespace game::render