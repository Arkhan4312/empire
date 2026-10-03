#include "ui/UIContext.h"

namespace game::ui {
// Resets hovered state at the beginning of a frame
void UIContext::beginFrame() {
    hovered = nullptr;
}
// clears active whien the mouse button is released
void UIContext::endFrame() {
    if (input && !input->mouseDown[mouse::Left]) {
        active = nullptr;
    }
}
}  // namespace game::ui