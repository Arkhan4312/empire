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
        return {available.x, thickness + marginV * 2.0f};
    }
    void render(UIContext& ctx) override;
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
};

// slider
class Slider : public Widget {
public:
    std::string label;
    float* value = nullptr;
    float minValue = 0.0f;
    float maxValue = 0.0f;
    float step = 0.05f;
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
    float scale = 1.0f;
    float labelWidth = 180.0f;
    float badgeWidth = 110.0f;

    KeyBindButton() = default;
    KeyBindButton(std::string a, int k) : action(std::move(a)), key(k) {
    }

    glm::vec2 measure(UIContext& ctx, const glm::vec2& available) override;
    void render(UIContext& ctx) override;
};
}  // namespace game::ui