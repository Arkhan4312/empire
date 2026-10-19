
#include <empire/engine/settings/Settings.h>
#include <empire/engine/settings/SettingsSystem.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <system_error>

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
void writeLevelToJson(json& j, empire::log::Level lvl) {
    j = empire::log::levelName(lvl);
}

std::optional<empire::log::Level> readLevelFromJson(const json& j) {
    if (!j.is_string()) {
        return std::nullopt;
    }
    return empire::log::ParseLevel(j.get<std::string>());
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
        auto seed = [&](const char* k, const char* busName) {
            if (it->contains(k) && (*it)[k].is_number()) {
                if (s.busVolumes.find(busName) == s.busVolumes.end()) {
                    s.busVolumes[busName] = readFloat(*it, k, 0.0f);
                }
            }
        };
        seed("masterVolume", "master");
        seed("sfxVolume", "sfx");
        seed("musicVolume", "music");

        // New format: nested "buses" object.
        if (auto b = it->find("buses"); b != it->end() && b->is_object()) {
            for (auto& [name, val] : b->items()) {
                if (!val.is_number()) {
                    continue;
                }
                float v = val.get<float>();
                if (v < 0.0f) {
                    v = 0.0f;
                }
                if (v > 1.0f) {
                    v = 1.0f;
                }
                s.busVolumes[name] = v;
            }
        }
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
    if (auto it = j.find("logging"); it != j.end() && it->is_object()) {
        if (auto ch = it->find("channels");
            ch != it->end() && ch->is_object()) {
            s.logChannelLevels.clear();
            for (auto& [key, val] : ch->items()) {
                if (auto lvl = readLevelFromJson(val)) {
                    s.logChannelLevels.emplace(key, *lvl);
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
    a["audioDeviceIndex"] = s.audioDeviceIndex;
    json busObj = json::object();
    for (const auto& [name, v] : s.busVolumes) {
        busObj[name] = v;
    }
    a["buses"] = std::move(busObj);
    j["audio"] = std::move(a);

    json keys = json::array();
    for (const auto& kb : s.keys) {
        keys.push_back({{"action", kb.action},
                        {"key", kb.key},
                        {"default", kb.defaultKey}});
    }
    j["controls"] = {{"keys", std::move(keys)}};

    json logging;
    logging["channels"] = json::object();
    for (const auto& [name, lvl] : s.logChannelLevels) {
        logging["channels"][name] = empire::log::levelName(lvl);
    }
    j["logging"] = std::move(logging);

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
