#pragma once
#include <string>

#include "render/Texture.h"

namespace game::render {

// stub. Will be added soon
class Font {
public:
    Font() = default;
    ~Font() = default;

    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;

    bool loadFromFile(const std::string& /*path*/, float /*pixelSize*/ = 16.0f);

    void destroy();

    bool valid() const noexcept {
        return m_loaded;
    }
    const Texture& atlas() const noexcept {
        return m_atlas;
    }

private:
    Texture m_atlas;
    bool m_loaded = false;
};
}  // namespace game::render