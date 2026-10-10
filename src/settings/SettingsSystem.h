#pragma once
#include <string>
namespace game {
struct Settings;

namespace settings {
bool load(Settings& s, const std::string& path);
bool save(const Settings& s, const std::string& path);
bool removeFile(const std::string& path);
}  // namespace settings
}  // namespace game