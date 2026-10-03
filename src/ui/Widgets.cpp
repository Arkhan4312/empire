#include "ui/Widgets.h"

#include <algorithm>

#include "core/Input.h"
#include "render/Font.h"
#include "render/Renderer.h"
#include "ui/UIContext.h"
namespace game::ui {
//-----------------------------Helpers---------------------------------
static int countVisible(const std::vector<WidgetPtr>& cs) {
    int n = 0;
    for (auto& c : cs) {
        if (c->visible) {
            ++n;
        }
    }
    return n;
}

static int countLines(const std::string& s) {
    int lines = 1;
    for (char ch : s) {
        if (ch == '\n') {
            ++lines;
        }
    }
    return lines;
}

//-----------------------------Layouts---------------------------------
//-------------------------Overlay layout------------------------------
glm::vec2 BoxLayout::measure(Container& c, UIContext& ctx,
                             const glm::vec2& available) {
    const float sp = spacing > 0.0f ? spacing : ctx.theme.spacing;
    const int n = countVisible(c.children);
    if (n == 0) {
        return {0.0f, 0.0f};
    }
    glm::vec2 total{0.0f, 0.0f};
    for (auto& child : c.children) {
        if (!child->visible) {
            continue;
        }
        const glm::vec2 s = child->measure(ctx, available);
        if (axis == MainAxis::Vertical) {
            total.y += s.y;
            total.x = std::max(total.x, s.x);
        } else {
            total.x += s.x;
            total.y = std::max(total.y, s.y);
        }
    }
    if (n > 1) {
        const float gap = sp * static_cast<float>(n - 1);
        if (axis == MainAxis::Vertical) {
            total.y += gap;
        } else {
            total.x += gap;
        }
    }
    return total;
}

void BoxLayout::arrange(Container& c, UIContext& ctx, const glm::vec2& pos,
                        const glm::vec2& size) {
    const float sp = spacing > 0.0f ? spacing : ctx.theme.spacing;

    std::vector<glm::vec2> sizes;
    sizes.reserve(c.children.size());
    for (auto& child : c.children) {
        sizes.push_back(child->visible ? child->measure(ctx, size)
                                       : glm::vec2{0.0f, 0.0f});
    }

    const bool vertical = (axis == MainAxis::Vertical);
    const float crossOrigin = vertical ? pos.x : pos.y;
    const float crossAvail = vertical ? size.x : size.y;
    float cursor = vertical ? pos.y : pos.x;

    for (std::size_t i = 0; i < c.children.size(); ++i) {
        auto& child = c.children[i];
        if (!child->visible) {
            continue;
        }
        const glm::vec2 s = sizes[i];
        const float mainChild = vertical ? s.y : s.x;
        float crossStart = crossOrigin;
        float crossSz = vertical ? s.x : s.y;

        switch (cross) {
            case CrossAlign::Start:
                break;
            case CrossAlign::Center:
                crossStart = crossOrigin + (crossAvail - crossSz) * 0.5f;
                break;
            case CrossAlign::End:
                crossStart = crossOrigin + (crossAvail - crossSz);
                break;
            case CrossAlign::Stretch:
                crossSz = crossAvail;
                break;
        }

        glm::vec2 cpos, csz;
        if (vertical) {
            cpos = {crossStart, cursor};
            csz = {crossSz, mainChild};
        } else {
            cpos = {cursor, crossStart};
            csz = {mainChild, crossSz};
        }
        child->arrange(ctx, cpos, csz);
        cursor += mainChild + sp;
    }
}
//-------------------------Grid layout---------------------------------
glm::vec2 GridLayout::measure(Container& c, UIContext& ctx,
                              const glm::vec2& available) {
    const int n = countVisible(c.children);
    if (n == 0) {
        return {0.0f, 0.0f};
    }

    const float hs = hSpacing > 0.0f ? hSpacing : ctx.theme.spacing;
    const float vs = vSpacing > 0.0f ? vSpacing : ctx.theme.spacing;
    const int cols = std::max(1, columns);
    const int rows = (n + cols - 1) / cols;

    float cellW = available.x > 0.0f
                      ? (available.x - hs * static_cast<float>(cols - 1)) /
                            static_cast<float>(cols)
                      : 0.0f;
    float cellH = available.y > 0.0f
                      ? (available.y - vs * static_cast<float>(rows - 1)) /
                            static_cast<float>(rows)
                      : 0.0f;
    if (cellW < 0.0f) {
        cellW = 0.0f;
    }
    if (cellH < 0.0f) {
        cellH = 0.0f;
    }

    float maxW = 0.0f;
    float maxH = 0.0f;
    for (auto& child : c.children) {
        if (!child->visible) {
            continue;
        }
        const glm::vec2 s = child->measure(ctx, {cellW, cellH});
        maxW = std::max(maxW, s.x);
        maxH = std::max(maxH, s.y);
    }
    return {
        maxW * static_cast<float>(cols) + hs * static_cast<float>(cols - 1),
        maxH * static_cast<float>(rows) + vs * static_cast<float>(rows - 1)};
}

void GridLayout::arrange(Container& c, UIContext& ctx, const glm::vec2& pos,
                         const glm::vec2& size) {
    const int n = countVisible(c.children);
    if (n == 0) return;

    const float hs = hSpacing > 0.0f ? hSpacing : ctx.theme.spacing;
    const float vs = vSpacing > 0.0f ? vSpacing : ctx.theme.spacing;
    const int cols = std::max(1, columns);
    const int rows = (n + cols - 1) / cols;

    const float cellW =
        (size.x - hs * static_cast<float>(cols - 1)) / static_cast<float>(cols);
    const float cellH =
        (size.y - vs * static_cast<float>(rows - 1)) / static_cast<float>(rows);

    int idx = 0;
    for (auto& child : c.children) {
        if (!child->visible) continue;
        const int r = idx / cols;
        const int col = idx % cols;

        glm::vec2 cpos{pos.x + static_cast<float>(col) * (cellW + hs),
                       pos.y + static_cast<float>(r) * (cellH + vs)};
        glm::vec2 csz{cellW, cellH};

        if (cross != CrossAlign::Stretch) {
            const glm::vec2 d = child->measure(ctx, csz);
            if (cross == CrossAlign::Center) {
                cpos.x += (cellW - d.x) * 0.5f;
                cpos.y += (cellH - d.y) * 0.5f;
            } else if (cross == CrossAlign::End) {
                cpos.x += (cellW - d.x);
                cpos.y += (cellH - d.y);
            }
            csz = d;
        }
        child->arrange(ctx, cpos, csz);
        ++idx;
    }
}

//-------------------------Overlay layout------------------------------
glm::vec2 OverlayLayout::measure(Container& c, UIContext& ctx,
                                 const glm::vec2& available) {
    glm::vec2 total{0.0f, 0.0f};
    for (auto& child : c.children) {
        if (!child->visible) continue;
        const glm::vec2 s = child->measure(ctx, available);
        total.x = std::max(total.x, s.x);
        total.y = std::max(total.y, s.y);
    }
    return total;
}

void OverlayLayout::arrange(Container& c, UIContext& ctx, const glm::vec2& pos,
                            const glm::vec2& size) {
    for (auto& child : c.children) {
        if (!child->visible) {
            continue;
        }
        child->arrange(ctx, pos, size);
    }
}

//------------------------------Container------------------------------
glm::vec2 Container::measure(UIContext& ctx, const glm::vec2& available) {
    const glm::vec2 innerAvail{std::max(0.0f, available.x - padding * 2.0f),
                               std::max(0.0f, available.y - padding * 2.0f)};

    glm::vec2 content{0.0f, 0.0f};
    if (layout) {
        content = layout->measure(*this, ctx, innerAvail);
    } else {
        for (auto& child : children) {
            if (!child->visible) {
                continue;
            }
            const glm::vec2 s = child->measure(ctx, innerAvail);
            content.x = std::max(content.x, s.x);
            content.y = std::max(content.y, s.y);
        }
    }
    return {content.x + padding * 2.0f, content.y + padding * 2.0f};
}

void Container::arrange(UIContext& ctx, const glm::vec2& pos,
                        const glm::vec2& size) {
    m_pos = pos;
    m_size = size;

    const glm::vec2 innerPos{pos.x + padding, pos.y + padding};
    const glm::vec2 innerSize{std::max(0.0f, size.x - padding * 2.0f),
                              std::max(0.0f, size.y - padding * 2.0f)};

    if (layout) {
        layout->arrange(*this, ctx, innerPos, innerSize);
    } else {
        for (auto& child : children) {
            if (!child->visible) {
                continue;
            }
            child->arrange(ctx, innerPos, innerSize);
        }
    }
}

void Container::update(UIContext& ctx) {
    if (!visible) {
        return;
    }
    for (auto& child : children) {
        child->update(ctx);
    }
}
void Container::render(UIContext& ctx) {
    if (!visible) {
        return;
    }
    if (drawBackground && ctx.renderer) {
        ctx.renderer->drawRect(m_pos, m_size, background);
        if (borderWidth > 0.0f) {
            const float bw = borderWidth;
            ctx.renderer->drawRect({m_pos.x, m_pos.y}, {m_size.x, bw},
                                   borderColor);
            ctx.renderer->drawRect({m_pos.x, m_pos.y + m_size.y - bw},
                                   {m_size.x, bw}, borderColor);
            ctx.renderer->drawRect({m_pos.x, m_pos.y}, {bw, m_size.y},
                                   borderColor);
            ctx.renderer->drawRect({m_pos.x + m_size.x - bw, m_pos.y},
                                   {bw, m_size.y}, borderColor);
        }
    }
    for (auto& child : children) {
        child->render(ctx);
    }
}
//------------------------------Label----------------------------------
glm::vec2 Label::measure(UIContext& ctx, const glm::vec2& available) {
    (void)available;
    if (!ctx.font || text.empty()) {
        return {0.0f, 0.0f};
    }

    float widest = 0.0f;
    float lineW = 0.0f;
    for (char ch : text) {
        if (ch == '\n') {
            widest = std::max(widest, lineW);
            lineW = 0.0f;
            continue;
        }
        if (ch == '\r') {
            continue;
        }
        const render::Glyph* g = ctx.font->glyph(ch);
        if (g) {
            lineW += g->xadvance;
        }
    }
    widest = std::max(widest, lineW);
    const float h =
        ctx.font->lineHeight() * static_cast<float>(countLines(text));
    return {widest * scale, h * scale};
}

void Label::render(UIContext& ctx) {
    if (!visible || !ctx.renderer || !ctx.font) {
        return;
    }
    const glm::vec4 c = useThemeColor ? ctx.theme.textColor : color;
    const glm::vec2 measured = measure(ctx, m_size);

    float x = m_pos.x;
    float y = m_pos.y;
    switch (halign) {
        case HAlign::Left:
            break;
        case HAlign::Center:
            x += (m_size.x - measured.x) * 0.5f;
            break;
        case HAlign::Right:
            x += (m_size.x - measured.x);
            break;
    }
    switch (valign) {
        case VAlign::Top:
            break;
        case VAlign::Middle:
            y += (m_size.y - measured.y) * 0.5f;
            break;
        case VAlign::Bottom:
            y += (m_size.y - measured.y);
            break;
    }
    ctx.renderer->drawText(*ctx.font, text, {x, y}, c, scale);
}

//------------------------------Image----------------------------------
glm::vec2 Image::measure(UIContext& /*ctx*/, const glm::vec2& available) {
    if (preferredSize.x > 0.0f || preferredSize.y > 0.0f) {
        return preferredSize;
    }
    if (texture && texture->valid()) {
        return {static_cast<float>(texture->width()),
                static_cast<float>(texture->height())};
    }
    return available;
}

void Image::render(UIContext& ctx) {
    if (!visible || !ctx.renderer || !texture || !texture->valid()) {
        return;
    }
    ctx.renderer->batch().drawUV(*texture, m_pos, m_size, uv, tint, 0.0f);
}

//------------------------------Button---------------------------------
glm::vec2 Button::measure(UIContext& ctx, const glm::vec2& /*available*/) {
    glm::vec2 content{0.0f, 0.0f};
    if (ctx.font && !text.empty()) {
        content.x = ctx.font->measureText(text) * scale;
        content.y = ctx.font->lineHeight() * scale;
    }
    glm::vec2 s{content.x + ctx.theme.padding * 2.0f,
                content.y + ctx.theme.padding * 1.5f};
    s.x = std::max(s.x, minSize.x);
    s.y = std::max(s.y, minSize.y);
    return s;
}

void Button::update(UIContext& ctx) {
    if (!visible || !enabled) {
        return;
    }
    const bool hovered = hitTest(ctx.mousePos());
    if (hovered) {
        ctx.hovered = this;
    }
    if (hovered && ctx.mousePressed()) {
        ctx.active = this;
    }
    if (ctx.active == this && ctx.mouseReleased()) {
        if (hovered && OnClick) {
            OnClick();
        }
        ctx.active = nullptr;
    }
}

void Button::render(UIContext& ctx) {
    if (!visible || !ctx.renderer) {
        return;
    }

    const bool hovered = (ctx.hovered == this);
    const bool pressed = (ctx.active == this);

    glm::vec4 bg = bgColor;
    if (useThemeBackground) {
        bg = ctx.theme.buttonBg;
        if (!enabled) {
            bg = ctx.theme.buttonDisabled;
        } else if (pressed) {
            bg = ctx.theme.buttonPressed;
        } else if (hovered) {
            bg = ctx.theme.buttonHover;
        }
    }
    ctx.renderer->drawRect(m_pos, m_size, bg);

    if (ctx.font && !text.empty()) {
        const float w = ctx.font->measureText(text) * scale;
        const float h = ctx.font->lineHeight() * scale;
        const glm::vec2 tp{m_pos.x + (m_size.x - w) * 0.5f,
                           m_pos.y + (m_size.y - h) * 0.5f};
        const glm::vec4 tc = useThemeText ? (enabled ? ctx.theme.textColor
                                                     : ctx.theme.textDisabled)
                                          : textColor;
        ctx.renderer->drawText(*ctx.font, text, tp, tc, scale);
    }
}
//-----------------------------Divider---------------------------------
void Divider::render(UIContext& ctx) {
    if (!visible || !ctx.renderer) return;
    ctx.renderer->drawRect({m_pos.x, m_pos.y + marginV}, {m_size.x, thickness},
                           color);
}
//-----------------------------Checkbox--------------------------------
glm::vec2 Checkbox::measure(UIContext& ctx, const glm::vec2& /*available*/) {
    const float box = ctx.font ? ctx.font->lineHeight() * scale : 20.0f;
    const float textW = (ctx.font && !text.empty())
                            ? ctx.font->measureText(text) * scale
                            : 0.0f;
    return {box + ctx.theme.spacing + textW, box};
}

void Checkbox::update(UIContext& ctx) {
    if (!visible || !enabled) {
        return;
    }
    const bool hovered = hitTest(ctx.mousePos());
    if (hovered) {
        ctx.hovered = this;
    }
    if (hovered && ctx.mousePressed()) {
        ctx.active = this;
    }
    if (ctx.active == this && ctx.mouseReleased()) {
        if (hovered && value) {
            *value = !*value;
        }
        ctx.active = nullptr;
    }
}

void Checkbox::render(UIContext& ctx) {
    if (!visible || !ctx.renderer) {
        return;
    }
    const float box = ctx.font ? ctx.font->lineHeight() * scale : 20.0f;
    const float by = m_pos.y + (m_size.y - box) * 0.5f;

    glm::vec4 bg = ctx.theme.buttonBg;
    if (!enabled) {
        bg = ctx.theme.buttonDisabled;
    } else if (ctx.hovered == this) {
        bg = ctx.theme.buttonHover;
    } else if (ctx.active == this) {
        bg = ctx.theme.buttonPressed;

        ctx.renderer->drawRect({m_pos.x, by}, {box, box}, bg);

        if (value && *value) {
            const float inset = box * 0.28f;
            ctx.renderer->drawRect({m_pos.x + inset, by + inset},
                                   {box - inset * 2.0f, box - inset * 2.0f},
                                   ctx.theme.textColor);
        }
        if (ctx.font && !text.empty()) {
            const float tx = m_pos.x + box + ctx.theme.spacing;
            const float th = ctx.font->lineHeight() * scale;
            const float ty = m_pos.y + (m_size.y - th) * 0.5f;
            ctx.renderer->drawText(
                *ctx.font, text, {tx, ty},
                enabled ? ctx.theme.textColor : ctx.theme.textDisabled, scale);
        }
    }
}
//----------------------------Choice row-------------------------------
glm::vec2 ChoiceRow::measure(UIContext& ctx, const glm::vec2& /*available*/) {
    const float lh = ctx.font ? ctx.font->lineHeight() * scale : 20.0f;
    const float rowH = lh + segmentPadding * 1.2f;

    const int n = static_cast<int>(options.size());
    const float segW = 80.0f;
    const float w = labelWidth + ctx.theme.spacing +
                    static_cast<float>(n) * segW +
                    spacing * static_cast<float>(std::max(0, n - 1));
    return {w, rowH};
}

void ChoiceRow::arrange(UIContext& ctx, const glm::vec2& pos,
                        const glm::vec2& size) {
    Widget::arrange(ctx, pos, size);
    m_segments.clear();

    const int n = static_cast<int>(options.size());
    if (n == 0) {
        return;
    }

    const float startX = pos.x + labelWidth + ctx.theme.spacing;
    const float total = size.x - labelWidth - ctx.theme.spacing;
    const float segW =
        (total - spacing * static_cast<float>(n - 1)) / static_cast<float>(n);

    m_segments.reserve(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        const float x = startX + static_cast<float>(i) * (segW + spacing);
        m_segments.push_back({x, pos.y, segW, size.y});
    }
}

void ChoiceRow::update(UIContext& ctx) {
    if (!visible || !enabled || !value) {
        return;
    }
    const glm::vec2 mp = ctx.mousePos();
    int hitIdx = -1;
    for (int i = 0; i < static_cast<int>(m_segments.size()); ++i) {
        const glm::vec4& r = m_segments[i];
        if (mp.x >= r.x && mp.x < r.x + r.z && mp.y >= r.y &&
            mp.y < r.y + r.w) {
            hitIdx = i;
            break;
        }
    }
    if (hitIdx >= 0) {
        ctx.hovered = this;
        if (ctx.mousePressed()) {
            *value = hitIdx;
        }
    }
}

void ChoiceRow::render(UIContext& ctx) {
    if (!visible || !ctx.renderer) {
        return;
    }
    if (ctx.font && !label.empty()) {
        const float lh = ctx.font->lineHeight() * scale;
        const float ly = m_pos.y + (m_size.y - lh) * 0.5f;
        ctx.renderer->drawText(
            *ctx.font, label, {m_pos.x, ly},
            enabled ? ctx.theme.textColor : ctx.theme.textDisabled, scale);
    }
    const int activeIdx = value ? *value : -1;

    for (int i = 0; i < static_cast<int>(m_segments.size()); ++i) {
        const glm::vec4& r = m_segments[i];
        const glm::vec2 segPos{r.x, r.y};
        const glm::vec2 segSize{r.z, r.w};

        glm::vec4 bg = ctx.theme.buttonBg;
        if (!enabled) {
            bg = ctx.theme.buttonDisabled;
        } else if (i == activeIdx) {
            bg = ctx.theme.buttonPressed;
        } else if (ctx.hovered == this) {
            bg = ctx.theme.buttonHover;
        }

        ctx.renderer->drawRect(segPos, segSize, bg);

        if (ctx.font && i < static_cast<int>(options.size())) {
            const std::string& opt = options[i];
            const float tw = ctx.font->measureText(opt) * scale;
            const float th = ctx.font->lineHeight() * scale;
            ctx.renderer->drawText(
                *ctx.font, opt,
                {r.x + (r.z - tw) * 0.5f, r.y + (r.w - th) * 0.5f},
                enabled ? ctx.theme.textColor : ctx.theme.textDisabled, scale);
        }
    }
}
//------------------------------Slider---------------------------------
glm::vec2 Slider::measure(UIContext& ctx, const glm::vec2& /*available*/) {
    const float lh = ctx.font ? ctx.font->lineHeight() * scale : 20.0f;
    const float w = labelWidth + ctx.theme.spacing + 160.0f +
                    ctx.theme.spacing + valueWidth;
    return {w, lh + 8.0f};
}

void Slider::arrange(UIContext& /*ctx*/, const glm::vec2& pos,
                     const glm::vec2& size) {
    m_pos = pos;
    m_size = size;
}

void Slider::update(UIContext& ctx) {
    if (!visible || !enabled || !value) {
        return;
    }
    const float trackX = m_pos.x + labelWidth + ctx.theme.spacing;
    const float trackW =
        std::max(10.0f, m_size.x - labelWidth - ctx.theme.spacing - valueWidth -
                            ctx.theme.spacing);
    const glm::vec2 mp = ctx.mousePos();

    const bool overRow = mp.x >= m_pos.x && mp.x < m_pos.x + m_size.x &&
                         mp.y >= m_pos.y && mp.y < m_pos.y + m_size.y;

    if (overRow) {
        ctx.hovered = this;
        if (ctx.mousePressed()) {
            ctx.active = this;
        }
    }

    if (ctx.active == this && ctx.mouseDown()) {
        float t = (mp.x - trackX) / trackW;
        if (t < 0.0f) {
            t = 0.0f;
        }
        if (t > 1.0f) {
            t = 1.0f;
        }
        float v = minValue + t * (maxValue - minValue);
        if (step > 0.0f) {
            v = minValue + std::round((v - minValue) / step) * step;
        }
        if (v < minValue) {
            v = minValue;
        }
        if (v > maxValue) {
            v = maxValue;
        }
        *value = v;
    }
}
void Slider::render(UIContext& ctx) {
    if (!visible || !ctx.renderer) {
        return;
    }
    const float lh = ctx.font ? ctx.font->lineHeight() * scale : 20.0f;
    const float cy = m_pos.y + m_size.y * 0.5f;

    const float trackX = m_pos.x + labelWidth + ctx.theme.spacing;
    const float trackW =
        std::max(10.0f, m_size.x - labelWidth - ctx.theme.spacing - valueWidth -
                            ctx.theme.spacing);

    // label
    if (ctx.font && !label.empty()) {
        const float ly = m_pos.y + (m_size.y - lh) * 0.5f;
        ctx.renderer->drawText(
            *ctx.font, label, {m_pos.x, ly},
            enabled ? ctx.theme.textColor : ctx.theme.textDisabled, scale);
    }

    // Track
    const float trackH = 6.0f;
    const float trackY = cy - trackH * 0.5f;
    ctx.renderer->drawRect({trackX, trackY}, {trackW, trackH},
                           {0.20f, 0.22f, 0.26f, 1.0f});

    // Fill
    float t = 0.0f;
    if (value && maxValue > minValue) {
        t = (*value - minValue) / (maxValue - minValue);
        if (t < 0.0f) {
            t = 0.0f;
        }
        if (t > 1.0f) {
            t = 1.0f;
        }
    }
    ctx.renderer->drawRect({trackX, trackY}, {trackW * t, trackH},
                           {0.40f, 0.65f, 0.95f, 1.0f});

    // Thumb
    const float thumbSize = 14.0f;
    const float thumbX = trackX + trackW * t - thumbSize * 0.5f;
    const float thumbY = cy - thumbSize * 0.5f;
    ctx.renderer->drawRect({thumbX, thumbY}, {thumbSize, thumbSize},
                           {0.90f, 0.92f, 0.96f, 1.0f});

    // Value
    if (ctx.font && value) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%d%%",
                      static_cast<int>(std::round(*value * 100.0f)));
        const float vx = m_pos.x + m_size.x - valueWidth + 4.0f;
        const float vy = m_pos.y + (m_size.y - lh) * 0.5f;
        ctx.renderer->drawText(*ctx.font, buf, {vx, vy}, ctx.theme.textColor,
                               scale);
    }
}
//--------------------------Key bind button----------------------------
glm::vec2 KeyBindButton::measure(UIContext& ctx,
                                 const glm::vec2& /*available*/) {
    const float lh = ctx.font ? ctx.font->lineHeight() * scale : 20.0f;
    return {labelWidth + ctx.theme.spacing + badgeWidth, lh + 8.0f};
}

void KeyBindButton::render(UIContext& ctx) {
    if (!visible || !ctx.renderer) {
        return;
    }

    const float lh = ctx.font ? ctx.font->lineHeight() * scale : 20.0f;

    if (ctx.font && !action.empty()) {
        const float ly = m_pos.y + (m_size.y - lh) * 0.5f;
        ctx.renderer->drawText(*ctx.font, action, {m_pos.x, ly},
                               ctx.theme.textColor, scale);
    }

    const float bx = m_pos.x + m_size.x - badgeWidth;
    const float by = m_pos.y + 2.0f;
    const float bh = m_size.y - 4.0f;

    ctx.renderer->drawRect({bx, by}, {badgeWidth, bh}, ctx.theme.buttonBg);

    if (ctx.font) {
        const std::string name = keyName(key);
        const float tw = ctx.font->measureText(name) * scale;
        const float th = ctx.font->lineHeight() * scale;
        ctx.renderer->drawText(
            *ctx.font, name,
            {bx + (badgeWidth - tw) * 0.5f, by + (bh - th) * 0.5f},
            ctx.theme.textColor, scale);
    }
}
}  // namespace game::ui