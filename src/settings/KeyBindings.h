#pragma once
#include <cstring>
#include <string>

#include "core/Input.h"

namespace game::settings {

struct KeyBindDefault {
    const char* action;
    int key;
};

inline constexpr KeyBindDefault kDefaults[] = {
    {"move_up", keys::W},     {"move_down", keys::S},  {"move_left", keys::A},
    {"move_right", keys::D},  {"interact", keys::E},   {"confirm", keys::Enter},
    {"cancel", keys::Escape}, {"pause", keys::Escape}, {"debug_tab", keys::Tab},
};

inline int defaultKeyFor(std::string_view action, int fallback = 0) {
    for (const auto& d : kDefaults) {
        if (action == d.action) {
            return d.key;
        }
    }
    return fallback;
}
}  // namespace game::settings