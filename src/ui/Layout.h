#pragma once
#include <glm/glm.hpp>

namespace game::ui {
class Container;
class UIContext;

enum class MainAxis {
    Horizontal,
    Vertical,
};
enum class CrossAlign {
    Start,
    Center,
    End,
    Stretch,
};

// Base interface for a layout
class Layout {
public:
    virtual ~Layout() = default;
    // Measures desired container size given available space.
    virtual glm::vec2 measure(Container& c, UIContext& ctx,
                              const glm::vec2& available) = 0;
    // Arranges children inside the given rect.
    virtual void arrange(Container& c, UIContext& ctx, const glm::vec2& pos,
                         const glm::vec2& size) = 0;
};
// Linear Box layout: vertical or horizontal
class BoxLayout : public Layout {
public:
    explicit BoxLayout(MainAxis a = MainAxis::Vertical,
                       CrossAlign c = CrossAlign::Start, float sp = 0.0f)
        : axis(a), cross(c), spacing(sp) {
    }

    MainAxis axis = MainAxis::Vertical;
    CrossAlign cross = CrossAlign::Start;
    float spacing = 0.0f;
    glm::vec2 measure(Container& c, UIContext& ctx,
                      const glm::vec2& available) override;
    void arrange(Container& c, UIContext& ctx, const glm::vec2& pos,
                 const glm::vec2& size) override;
};
// GridLayout
class GridLayout : public Layout {
public:
    explicit GridLayout(int cols = 2, float hs = 0.0f, float vs = 0.0f,
                        CrossAlign c = CrossAlign::Stretch)
        : columns(cols), hSpacing(hs), vSpacing(vs), cross(c) {
    }

    int columns = 2;
    float hSpacing = 0.0f;
    float vSpacing = 0.0f;
    CrossAlign cross = CrossAlign::Stretch;

    glm::vec2 measure(Container& c, UIContext& ctx,
                      const glm::vec2& available) override;
    void arrange(Container& c, UIContext& ctx, const glm::vec2& pos,
                 const glm::vec2& size) override;
};
// Overlay: all children get the same rect.
class OverlayLayout : public Layout {
public:
    glm::vec2 measure(Container& c, UIContext& ctx,
                      const glm::vec2& available) override;
    void arrange(Container& c, UIContext& ctx, const glm::vec2& pos,
                 const glm::vec2& size) override;
};
}  // namespace game::ui