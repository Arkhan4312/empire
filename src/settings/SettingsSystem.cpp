#include "settings/SettingsSystem.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <system_error>

#include "settings/Settings.h"

namespace game::settings {
using nlohmann::json;

namespace {
bool readBool(const json& o, const char* k, bool fb) {
    if (!o.contains(k) || !o[k].is_boolean()) {
        return fb;
    }
    return o[k].get<bool>();
}
int readInt(const json& o, const char* k, int fb) {
    if (!o.contains(k) || !o[k].is_number_integer()) {
        return fb;
    }
    return o[k].get<int>();
}
float readFloat(const json& o, const char* k, float fb) {
    if (!o.contains(k) || !o[k].is_number()) {
        return fb;
    }
    return o[k].get<float>();
}
std::string readString(const json& o, const char* k, const std::string& fb) {
    if (!o.contains(k) || !o[k].is_string()) {
        return fb;
    }
    return o[k].get<std::string>();
}
}  // namespace

bool load(Settings& s, const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        return false;
    }
    json j;
    try {
        f >> j;
    } catch (...) {
        return false;
    }
    if (!j.is_object()) {
        return false;
    }

    // Graphics
    if (auto it = j.find("graphics"); it != j.end() && it->is_object()) {
        s.vsync = readBool(*it, "vsync", s.vsync);
        s.showFPS = readBool(*it, "showFPS", s.showFPS);
        s.resolutionIndex = readInt(*it, "resolutionIndex", s.resolutionIndex);
        s.qualityIndex = readInt(*it, "qualityIndex", s.qualityIndex);
        s.uiScale = readFloat(*it, "uiScale", s.uiScale);
        int wm = readInt(*it, "windowMode", static_cast<int>(s.windowMode));
        if (wm < 0 || wm > 2) {
            wm = 0;
        }
        s.windowMode = static_cast<WindowMode>(wm);
        s.monitorIndex = readInt(*it, "monitorIndex", s.monitorIndex);
        s.brightness = readFloat(*it, "brightness", s.brightness);
    }

    // Audio
    if (auto it = j.find("audio"); it != j.end() && it->is_object()) {
        s.audioDeviceIndex =
            readInt(*it, "audioDeviceIndex", s.audioDeviceIndex);
        s.masterVolume = readFloat(*it, "masterVolume", s.masterVolume);
        s.sfxVolume = readFloat(*it, "sfxVolume", s.sfxVolume);
        s.musicVolume = readFloat(*it, "musicVolume", s.musicVolume);
    }

    // Controls
    if (auto it = j.find("controls"); it != j.end() && it->is_object()) {
        if (auto it2 = it->find("keys"); it2 != it->end() && it2->is_array()) {
            for (auto& kb : *it2) {
                if (!kb.is_object()) {
                    continue;
                }
                const std::string action = readString(kb, "action", "");
                const int key = readInt(kb, "key", 0);
                const int def = readInt(kb, "default", 0);
                if (action.empty()) {
                    continue;
                }
                for (auto& e : s.keys) {
                    if (e.action == action) {
                        e.key = key;
                        e.defaultKey = def;
                        break;
                    }
                }
            }
        }
    }

    // Clamp
    if (s.uiScale < 0.75f) {
        s.uiScale = 0.75f;
    }
    if (s.uiScale > 2.00f) {
        s.uiScale = 2.00f;
    }
    if (s.resolutionIndex < 0 || s.resolutionIndex >= kResolutionCount) {
        s.resolutionIndex = 0;
    }
    if (s.qualityIndex < 0 || s.qualityIndex >= kQualityCount) {
        s.qualityIndex = 2;
    }
    if (s.brightness < 0.50f) {
        s.brightness = 0.50f;
    }
    if (s.brightness > 1.50f) {
        s.brightness = 1.50f;
    }
    if (s.monitorIndex < 0) {
        s.monitorIndex = 0;
    }
    if (s.masterVolume < 0.0f) {
        s.masterVolume = 0.0f;
    }
    if (s.masterVolume > 1.0f) {
        s.masterVolume = 1.0f;
    }
    if (s.sfxVolume < 0.0f) {
        s.sfxVolume = 0.0f;
    }
    if (s.sfxVolume > 1.0f) {
        s.sfxVolume = 1.0f;
    }
    if (s.musicVolume < 0.0f) {
        s.musicVolume = 0.0f;
    }
    if (s.musicVolume > 1.0f) {
        s.musicVolume = 1.0f;
    }
    return true;
}
bool save(const Settings& s, const std::string& path) {
    json j;
    j["version"] = 1;

    json g;
    g["vsync"] = s.vsync;
    g["windowMode"] = static_cast<int>(s.windowMode);
    g["monitorIndex"] = s.monitorIndex;
    g["brightness"] = s.brightness;
    g["showFPS"] = s.showFPS;
    g["resolutionIndex"] = s.resolutionIndex;
    g["qualityIndex"] = s.qualityIndex;
    g["uiScale"] = s.uiScale;
    j["graphics"] = std::move(g);

    json a;
    a["deviceIndex"] = s.audioDeviceIndex;
    a["masterVolume"] = s.masterVolume;
    a["sfxVolume"] = s.sfxVolume;
    a["musicVolume"] = s.musicVolume;
    j["audio"] = std::move(a);

    json keys = json::array();
    for (const auto& kb : s.keys) {
        keys.push_back({{"action", kb.action},
                        {"key", kb.key},
                        {"default", kb.defaultKey}});
    }
    j["controls"] = {{"keys", std::move(keys)}};

    const std::string tmp = path + ".tmp";
    {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        if (!out) {
            return false;
        }
        out << j.dump(2);
        out.flush();
        if (!out.good()) {
            return false;
        }
    }

    std::error_code ec;
    std::filesystem::rename(tmp, path, ec);
    if (ec) {
        std::error_code rm;
        std::filesystem::remove(tmp, rm);
        return false;
    }
    return true;
}

bool removeFile(const std::string& path) {
    std::error_code ec;
    return std::filesystem::remove(path, ec);
}
}  // namespace game::settings
