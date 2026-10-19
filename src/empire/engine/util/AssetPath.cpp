#include <empire/engine/util/AssetPath.h>
#include <filesystem>

namespace empire::asset {
namespace {
std::filesystem::path g_root;
}

void setRoot(std::filesystem::path root) {
    g_root = std::move(root);
}

const std::filesystem::path& root() noexcept {
    return g_root;
}

std::filesystem::path resolve(std::string_view logicalPath) {
    std::filesystem::path p(logicalPath);
    if (p.is_absolute()) {
        return p;
    }
    return g_root / p;
}

std::string toString(std::string_view logicalPath) {
    return resolve(logicalPath).string();
}
}  // namespace empire::asset
