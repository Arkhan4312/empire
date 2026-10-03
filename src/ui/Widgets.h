#pragma once
#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

#include "render/Texture.h"
#include "ui/Layout.h"
#include "ui/Widget.h"

namespace game::ui {
// container. Holds children and an optional layout.
// if layout == nullptr, children receive the same rectangle and the measured
// size is the maximum across the children

class Container : public Widget {
public:
    std::vector<WidgetPtr> children;
    std::unique_ptr<Layout> layout;
    bool drawBackground = false;
    glm::vec4 background{0.10f, 0.11f, 0.14f, 1.0f};
    glm::vec4 borderColor{0.25f, 0.27f, 0.32f, 1.0f};
    float borderWidth = 0.0f;

    float padding = 0.0f;
    glm::vec2 measure(UIContext& ctx, const glm::vec2& available) override;
    void arrange(UIContext& ctx, const glm::vec2& pos,
                 const glm::vec2& size) override;
    void update(UIContext& ctx) override;
    void render(UIContext& ctx) override;
    // Add new child
    template <typename T, typename... Args>
    T* add(Args&&... args) {
        auto w = std::make_unique<T>(std::forward<Args>(args)...);
        T* raw = w.get();
        children.push_back(std::move(w));
        return raw;
    }

    void clear() {
        children.clear();
    }
};

// Spacer. Fixed size empty widget, only for spacing.
class Spacer : public Widget {
public:
    explicit Spacer(glm::vec2 s = {0.0f, 0.0f}) : preferred(s) {
    }
    glm::vec2 preferred{0.0f, 0.0f};
    glm::vec2 measure(UIContext&, const glm::vec2&) override {
        return preferred;
    }
    void render(UIContext&) override {
    }
};
// Text or empty label.
class Label : public Widget {
public:
    enum class HAlign {
        Left,
        Center,
        Right,
    };
    enum class VAlign {
        Top,
        Middle,
        Bottom,
    };
    std::string text;
    float scale = 1.0f;
    glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
    bool useThemeColor = true;
    HAlign halign = HAlign::Left;
    VAlign valign = VAlign::Middle;

    Label() = default;
    explicit Label(std::string t) : text(std::move(t)) {
    }
    glm::vec2 measure(UIContext& ctx, const glm::vec2& available) override;
    void render(UIContext& ctx) override;
};
// Image
class Image : public Widget {
public:
    const render::Texture* texture = nullptr;
    glm::vec4 tint{1.0f, 1.0f, 1.0f, 1.0f};
    glm::vec4 uv{0.0f, 0.0f, 1.0f, 1.0f};
    glm::vec2 preferredSize{0.0f, 0.0f};
    glm::vec2 measure(UIContext& ctx, const glm::vec2& available) override;
    void render(UIContext& ctx) override;
};

// Button
class Button : public Widget {
public:
    std::string text;
    std::function<void()> OnClick;
    float scale = 1.0f;
    glm::vec2 minSize{120.0f, 32.0f};

    bool useThemeBackground = true;
    glm::vec4 bgColor{1.0f, 1.0f, 1.0f, 1.0f};
    bool useThemeText = true;
    glm::vec4 textColor{1.0f, 1.0f, 1.0f, 1.0f};

    Button() = default;
    explicit Button(std::string t) : text(std::move(t)) {
    }
    glm::vec2 measure(UIContext& ctx, const glm::vec2& available) override;
    void update(UIContext& ctx) override;
    void render(UIContext& ctx) override;
};
// thick horizontal divider-line
class Divider : public Widget {
public:
    float thickness = 1.0f;
    float marginV = 5.0f;
    glm::vec4 color{0.25f, 0.27f, 0.32f, 1.0f};

    glm::vec2 measure(UIContext&, const glm::vec2& available) override {
        const float w = available.x > 0.0f ? available.x : 1.0f;
        return {w, thickness + marginV * 2.0f};
    }
    void render(UIContext& ctx) override;
};
// Scroll
class ScrollView : public Widget {
public:
    Widget* child = nullptr;

    float scrollSpeed = 48.0f;

    bool drawScrollbar = true;
    float scrollbarWidth = 6.0f;
    float scrollbarMargin = 4.0f;

    bool drawBackground = false;
    glm::vec4 background{0.10f, 0.11f, 0.14f, 1.0f};
    glm::vec4 borderColor{0.25f, 0.27f, 0.32f, 1.0f};
    float borderWidth = 0.0f;

    glm::vec4 scrollbarTrackColor{0.10f, 0.11f, 0.14f, 1.0f};
    glm::vec4 scrollbarThumbColor{0.40f, 0.46f, 0.58f, 1.0f};

    glm::vec2 measure(UIContext& ctx, const glm::vec2& available) override;
    void arrange(UIContext& ctx, const glm::vec2& pos,
                 const glm::vec2& size) override;
    void update(UIContext& ctx) override;
    void render(UIContext& ctx) override;

    float scroll() const noexcept {
        return m_scroll;
    }
    bool scrollable() const noexcept {
        return m_maxScroll > 0.0f;
    }

private:
    void clampScroll() noexcept;
    void layoutChild(UIContext& ctx);

    float m_scroll = 0.0f;
    float m_contentHeight = 0.0f;
    float m_maxScroll = 0.0f;

    float m_thumbY = 0.0f;
    float m_thumbH = 0.0f;
    bool m_draggingThumb = false;
    float m_dragAnchorY = 0.0f;
};

// checkbox
class Checkbox : public Widget {
public:
    std::string text;
    bool* value = nullptr;
    float scale = 1.0f;
    Checkbox() = default;
    explicit Checkbox(std::string t) : text(std::move(t)) {
    }

    glm::vec2 measure(UIContext& ctx, const glm::vec2& available) override;
    void update(UIContext& ctx) override;
    void render(UIContext& ctx) override;
};

// label -> choice
class ChoiceRow : public Widget {
public:
    std::string label;
    std::vector<std::string> options;
    int* value = nullptr;
    float scale = 1.0f;
    float labelWidth = 140.0f;
    float segmentPadding = 8.0f;
    float spacing = 4.0f;

    glm::vec2 measure(UIContext& ctx, const glm::vec2& available) override;
    void arrange(UIContext& ctx, const glm::vec2& pos,
                 const glm::vec2& size) override;
    void update(UIContext& ctx) override;
    void render(UIContext& ctx) override;

private:
    std::vector<glm::vec4> m_segments;
    int m_hoveredSegment = -1;
};

// slider
class Slider : public Widget {
public:
    std::string label;
    float* value = nullptr;
    float minValue = 0.0f;
    float maxValue = 1.0f;
    float step = 0.01f;
    float scale = 1.0f;
    float labelWidth = 140.0f;
    float valueWidth = 60.0f;

    glm::vec2 measure(UIContext& ctx, const glm::vec2& available) override;
    void arrange(UIContext& ctx, const glm::vec2& pos,
                 const glm::vec2& size) override;
    void update(UIContext& ctx) override;
    void render(UIContext& ctx) override;
};
// Key bind button

class KeyBindButton : public Widget {
public:
    std::string action;
    int key = 0;
    int defaultKey = 0;
    float scale = 1.0f;
    float labelWidth = 180.0f;
    float badgeWidth = 110.0f;

    std::function<void(int)> onKeyChanged;

    KeyBindButton() = default;
    explicit KeyBindButton(std::string a) : action(std::move(a)) {
    }
    KeyBindButton(std::string a, int k)
        : action(std::move(a)), key(k), defaultKey(k) {
    }
    KeyBindButton(std::string a, int k, int def)
        : action(std::move(a)), key(k), defaultKey(def) {
    }

    glm::vec2 measure(UIContext& ctx, const glm::vec2& available) override;
    void update(UIContext& ctx) override;
    void render(UIContext& ctx) override;

    void setKey(int k) {
        if (key == k) {
            return;
        }
        key = k;
        if (onKeyChanged) {
            onKeyChanged(key);
        }
    }
    void resetToDefault() {
        setKey(defaultKey);
    }
    bool waitingForKey() {
        return m_waitingForKey;
    }

private:
    bool m_waitingForKey = false;
};
}  // namespace game::ui