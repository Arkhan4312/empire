#pragma once
#include <string>
#include <vector>

#include "core/Input.h"
#include "settings/WindowMode.h"

namespace game {

struct KeyBind {
    std::string action;
    int key = 0;
    int defaultKey = 0;
};
inline std::vector<KeyBind> defaultKeyBinds() {
    return {
        {"Click", keys::Space, keys::Space},   // space
        {"Buy upgrade", keys::E, keys::E},     // E
        {"Craft soldier", '1', '1'},           // 1
        {"Craft tank", '2', '2'},              // 2
        {"Craft plane", '3', '3'},             // 3
        {"Back", keys::Escape, keys::Escape},  // escape
    };
}

struct Settings {
    // Graphics
    WindowMode windowMode = WindowMode::Windowed;
    int monitorIndex = 0;
    bool vsync = true;
    bool showFPS = false;
    int resolutionIndex = 0;
    int qualityIndex = 2;
    float uiScale = 1.0f;
    float brightness = 1.0f;
    // Audio
    int audioDeviceIndex = -1;
    float masterVolume = 0.80f;
    float sfxVolume = 1.00f;
    float musicVolume = 0.60f;
    // Controls
    std::vector<KeyBind> keys = defaultKeyBinds();
    static Settings defaults() {
        return Settings{};
    }
};

struct Resolution {
    int w;
    int h;
};

inline constexpr Resolution kResolutions[] = {
    {1280, 720}, {1600, 900}, {1920, 1080}};
inline constexpr const char* kResolutionNames[] = {"1280x720", "1600x900",
                                                   "1920x1080"};
inline constexpr int kResolutionCount = 3;

inline constexpr const char* kQualityNames[] = {"Low", "Medium", "High",
                                                "Ultra"};
inline constexpr int kQualityCount = 4;

}  // namespace game