#pragma once
#include <string>
#include <vector>

namespace game {

struct KeyBind {
    std::string action;
    int key = 0;
    int defaultKey = 0;
};

struct Settings {
    // Graphics
    bool vsync = true;
    bool fullscreen = false;
    bool showFPS = false;
    int resolutionIndex = 0;
    int qualityIndex = 2;
    // Audio
    float masterVolume = 0.80f;
    float sfxVolume = 1.00f;
    float musicVolume = 0.60f;
    // Controls
    std::vector<KeyBind> keys = {
        {"Click", 32},          // space
        {"Buy upgrade", 69},    // E
        {"Craft soldier", 49},  // 1
        {"Craft tank", 50},     // 2
        {"Craft plane", 51},    // 3
        {"Back", 256},          // escape
    };
};

inline constexpr const char* kResolutionNames[] = {"1280 x 720", "1600 x 900",
                                                   "1920 x 1080"};
inline constexpr int kResolutionCount = 3;
inline constexpr const char* kQualityNames[] = {"Low", "Medium", "High",
                                                "Ultra"};
inline constexpr int kQualityCount = 4;

}  // namespace game