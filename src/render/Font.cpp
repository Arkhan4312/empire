#include "render/Font.h"

#include <cstdio>

namespace game::render {
bool Font::loadFromFile(const std::string& path, float pixelSize) {
    // stub
    std::fprintf(stderr, "[Font] (stub) not loading '%s' (size = %.1f)\n",
                 path.c_str(), pixelSize);
    m_loaded = false;

    return false;
}
void Font::destroy() {
    m_atlas.destroy();
    m_loaded = false;
}
}  // namespace game::render