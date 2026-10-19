#pragma once
#include <filesystem>
#include <string>
#include <string_view>

namespace empire::asset {
void setRoot(std::filesystem::path root);

const std::filesystem::path& root() noexcept;

std::filesystem::path resolve(std::string_view logicalPath);

std::string toString(std::string_view logicalPath);
}  // namespace empire::asset