#include "render/Font.h"

#include <stb_truetype.h>

#include <cstdio>
#include <fstream>
#include <vector>

namespace game::render {
bool Font::loadFromFile(const std::string& path, float pixelSize) {
    destroy();
    if (pixelSize < 4.0f) {
        pixelSize = 4.0f;
    }
    std::vector<unsigned char> ttf;
    {
        std::ifstream f(path, std::ios::binary);
        if (!f) {
            std::fprintf(stderr, "[Font]  cannot  open '%s'\n", path.c_str());
            return false;
        }
        f.seekg(0, std::ios::end);
        const std::streamoff size = f.tellg();
        if (size <= 0) {
            std::fprintf(stderr, "[Font] '%s; is empty\n", path.c_str());
            return false;
        }
        f.seekg(0, std::ios::beg);
        ttf.resize(static_cast<std::size_t>(size));
        if (!f.read(reinterpret_cast<char*>(ttf.data()), size)) {
            std::fprintf(stderr, "[Font] read failed '%s'\n", path.c_str());
            return false;
        }
    }
    const int offset = stbtt_GetFontOffsetForIndex(ttf.data(), 0);
    if (offset < 0) {
        std::fprintf(stderr, "[Font] not a TTF: '%s'\n", path.c_str());
        return false;
    }

    stbtt_fontinfo info{};
    if (!stbtt_InitFont(&info, ttf.data(), offset)) {
        std::fprintf(stderr, "[Font]  stbtt_InitFont failed '%s' \n",
                     path.c_str());
        return false;
    }

    int a = 0;
    int d = 0;
    int lg = 0;
    stbtt_GetFontVMetrics(&info, &a, &d, &lg);
    const float scale = stbtt_ScaleForPixelHeight(&info, pixelSize);
    m_ascent = static_cast<float>(a) * scale;
    m_descent = static_cast<float>(d) * scale;
    m_lineHeight = static_cast<float>(a - d + lg) * scale;
    m_pixelSize = pixelSize;

    constexpr int kAtlasW = 512;
    constexpr int kAtlasH = 512;
    constexpr int kPadding = 1;

    std::vector<unsigned char> bitmap(
        static_cast<std::size_t>(kAtlasW) * kAtlasH, 0);
    stbtt_packedchar packed[kGlyphCount]{};

    stbtt_pack_context pc;
    if (!stbtt_PackBegin(&pc, bitmap.data(), kAtlasW, kAtlasH, 0, kPadding,
                         nullptr)) {
        std::fprintf(stderr, "[Font] stbtt_PackBegin failed\n");
        return false;
    }
    stbtt_PackSetOversampling(&pc, 1, 1);
    const int ok = stbtt_PackFontRange(&pc, ttf.data(), 0, pixelSize,
                                       kFirstChar, kGlyphCount, packed);
    stbtt_PackEnd(&pc);
    if (!ok) {
        std::fprintf(stderr, "[Font] stbtt_PackFontRange failed\n");
        return false;
    }

    for (int i = 0; i < kGlyphCount; ++i) {
        const stbtt_packedchar& p = packed[i];
        Glyph& g = m_glyphs[i];
        g.u0 = static_cast<float>(p.x0) / static_cast<float>(kAtlasW);
        g.v0 = static_cast<float>(p.y0) / static_cast<float>(kAtlasH);
        g.u1 = static_cast<float>(p.x1) / static_cast<float>(kAtlasW);
        g.v1 = static_cast<float>(p.y1) / static_cast<float>(kAtlasH);
        g.width = p.x1 - p.x0;
        g.height = p.y1 - p.y0;
        g.xoff = p.xoff;
        g.yoff = p.yoff;
        g.xadvance = p.xadvance;
    }

    std::vector<unsigned char> rgba(
        static_cast<std::size_t>(kAtlasW) * kAtlasH * 4u, 0);
    for (std::size_t i = 0; i < bitmap.size(); ++i) {
        rgba[i * 4 + 0] = 255;
        rgba[i * 4 + 1] = 255;
        rgba[i * 4 + 2] = 255;
        rgba[i * 4 + 3] = bitmap[i];
    }

    if (!m_atlas.createRGBA(kAtlasW, kAtlasH, rgba.data())) {
        std::fprintf(stderr, "[Font] atlas upload failed\n");
        return false;
    }
    m_loaded = true;
    return true;
}
void Font::destroy() {
    m_atlas.destroy();
    m_loaded = false;
    m_pixelSize = 0.0f;
    m_ascent = 0.0f;
    m_descent = 0.0f;
    m_lineHeight = 0.0f;
    for (auto& g : m_glyphs) {
        g = Glyph{};
    }
}
const Glyph* Font::glyph(char c) const noexcept {
    const unsigned char uc = static_cast<unsigned char>(c);
    if (uc < kFirstChar || uc > kLastChar) {
        return nullptr;
    }
    return &m_glyphs[uc - kFirstChar];
}
float Font::measureText(std::string_view text) const noexcept {
    float w = 0.0f;
    float maxW = 0.0f;
    for (char c : text) {
        if (c == '\n') {
            if (w > maxW) {
                maxW = w;
            }
            w = 0.0f;
            continue;
        }
        if (c == '\r') {
            continue;
        }
        const Glyph* g = glyph(c);
        if (!g) {
            continue;
        }
        w += g->xadvance;
    }
    return (w > maxW) ? w : maxW;
}
}  // namespace game::render