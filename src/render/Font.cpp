#include "render/Font.h"

#include <stb_truetype.h>

#include <cstdio>
#include <fstream>
#include <vector>

namespace game::render {

std::uint32_t utf8Decode(std::string_view s, std::size_t& i) noexcept {
    const unsigned char c0 = static_cast<unsigned char>(s[i]);
    if (c0 < 0x80) {
        ++i;
        return c0;
    }
    if ((c0 & 0xE0) == 0xC0 && i + 1 < s.size()) {
        const unsigned char c1 = static_cast<unsigned char>(s[i + 1]);
        if ((c1 & 0xC0) == 0x80) {
            i += 2;
            return (static_cast<std::uint32_t>(c0 & 0x1F) << 6) |
                   static_cast<std::uint32_t>(c1 & 0x3F);
        }
    }
    if ((c0 & 0xF0) == 0xE0 && i + 2 < s.size()) {
        const unsigned char c1 = static_cast<unsigned char>(s[i + 1]);
        const unsigned char c2 = static_cast<unsigned char>(s[i + 2]);
        if ((c1 & 0xC0) == 0x80 && (c2 & 0xC0) == 0x80) {
            i += 3;
            return (static_cast<std::uint32_t>(c0 & 0x0F) << 12) |
                   (static_cast<std::uint32_t>(c1 & 0x3F) << 6) |
                   static_cast<std::uint32_t>(c2 & 0x3F);
        }
    }
    if ((c0 & 0xF8) == 0xF0 && i + 3 < s.size()) {
        const unsigned char c1 = static_cast<unsigned char>(s[i + 1]);
        const unsigned char c2 = static_cast<unsigned char>(s[i + 2]);
        const unsigned char c3 = static_cast<unsigned char>(s[i + 3]);
        if ((c1 & 0xC0) == 0x80 && (c2 & 0xC0) == 0x80 && (c3 & 0xC0) == 0x80) {
            i += 4;
            return (static_cast<std::uint32_t>(c0 & 0x07) << 18) |
                   (static_cast<std::uint32_t>(c1 & 0x3F) << 12) |
                   (static_cast<std::uint32_t>(c2 & 0x3F) << 6) |
                   static_cast<std::uint32_t>(c3 & 0x3F);
        }
    }
    ++i;
    return 0xFFFD;
}

namespace {
std::vector<int> buildCodepointSet() {
    std::vector<int> cps;
    cps.reserve(256);

    for (int c = 32; c <= 126; ++c) {
        cps.push_back(c);  // ASCII printable
    }

    cps.push_back(0x0401);
    for (int c = 0x0410; c <= 0x044F; ++c) {
        cps.push_back(c);  // Cyrillic
    }
    cps.push_back(0x0451);

    cps.push_back(0x2013);  // en dash
    cps.push_back(0x2014);  // em dash
    cps.push_back(0x00AB);  // left double quote
    cps.push_back(0x00BB);  // right double quote
    cps.push_back(0x2026);  // ellipsis
    return cps;
}
}  // namespace

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

    constexpr int kAtlasW = 1024;
    constexpr int kAtlasH = 1024;
    constexpr int kPadding = 1;

    std::vector<unsigned char> bitmap(
        static_cast<std::size_t>(kAtlasW) * kAtlasH, 0);

    std::vector<int> codepoints = buildCodepointSet();
    stbtt_pack_range range{};
    range.font_size = pixelSize;
    range.first_unicode_codepoint_in_range = 0;
    range.array_of_unicode_codepoints = codepoints.data();
    range.num_chars = static_cast<int>(codepoints.size());

    std::vector<stbtt_packedchar> packed(codepoints.size());
    range.chardata_for_range = packed.data();
    stbtt_pack_context pc;
    if (!stbtt_PackBegin(&pc, bitmap.data(), kAtlasW, kAtlasH, 0, kPadding,
                         nullptr)) {
        std::fprintf(stderr, "[Font] stbtt_PackBegin failed\n");
        return false;
    }
    stbtt_PackSetOversampling(&pc, 1, 1);
    const int ok = stbtt_PackFontRanges(&pc, ttf.data(), 0, &range, 1);
    stbtt_PackEnd(&pc);
    if (!ok) {
        std::fprintf(stderr, "[Font] stbtt_PackFontRange failed\n");
        return false;
    }
    m_glyphs.reserve(codepoints.size());
    for (int i = 0; i < codepoints.size(); ++i) {
        const stbtt_packedchar& p = packed[i];
        Glyph g;
        g.u0 = static_cast<float>(p.x0) / static_cast<float>(kAtlasW);
        g.v0 = static_cast<float>(p.y0) / static_cast<float>(kAtlasH);
        g.u1 = static_cast<float>(p.x1) / static_cast<float>(kAtlasW);
        g.v1 = static_cast<float>(p.y1) / static_cast<float>(kAtlasH);
        g.width = p.x1 - p.x0;
        g.height = p.y1 - p.y0;
        g.xoff = p.xoff;
        g.yoff = p.yoff;
        g.xadvance = p.xadvance;
        m_glyphs.emplace(static_cast<std::uint32_t>(codepoints[i]), g);
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
    m_glyphs.clear();
}
const Glyph* Font::glyph(std::uint32_t codepoint) const noexcept {
    auto it = m_glyphs.find(codepoint);
    return it == m_glyphs.end() ? nullptr : &it->second;
}
float Font::measureText(std::string_view text) const noexcept {
    float widest = 0.0f;
    float lineW = 0.0f;
    std::size_t i = 0;
    while (i < text.size()) {
        const std::uint32_t cp = utf8Decode(text, i);
        if (cp == '\n') {
            if (lineW > widest) {
                widest = lineW;
                lineW = 0.0f;
                continue;
            }
        }
        if (cp == '\r') {
            continue;
        }
        if (const Glyph* g = glyph(cp)) {
            lineW += g->xadvance;
        }
    }
    return lineW > widest ? lineW : widest;
}
}  // namespace game::render