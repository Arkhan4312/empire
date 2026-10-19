#include <empire/engine/log/LogSink.h>

#include <array>
#include <cstdio>
#include <filesystem>
#include <system_error>
#include <utility>

#if defined(_WIN32)
#include <io.h>
#define GAME_LOG_ISATTY _isatty
#define GAME_LOG_FILEN0 _fileno
#else
#include <unistd.h>
#define GAME_LOG_ISATTY isatty
#define GAME_LOG_FILEN0 fileno
#endif

namespace empire::log {
namespace {

constexpr std::array<std::string_view, 7> kLevelNames{
    "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "CRIT", "OFF",
};
constexpr std::array<std::string_view, 7> kLevelColors{
    "\x1b[90m",       // Trace
    "\x1b[36m",       // Debug
    "\x1b[0m",        // Info
    "\x1b[33m",       // Warn
    "\x1b[31m",       // Error
    "\x1b[1;97;41m",  // Critical
    "\x1b[0m",        // Off
};
constexpr std::string_view kReset = "\x1b[0m";
bool stdoutIsTty() noexcept {
    return GAME_LOG_ISATTY(GAME_LOG_FILEN0(stdout)) != 0;
}
}  // namespace

// NullSink - header-only

// ConsoleSink

const char* levelName(Level lvl) noexcept {
    const int idx = static_cast<int>(lvl);
    if (idx < 0 || idx >= static_cast<int>(kLevelNames.size())) {
        return "?";
    }
    return kLevelNames[static_cast<std::size_t>(idx)].data();
}

std::optional<Level> ParseLevel(std::string_view name) noexcept {
    auto eqIgnoreCase = [](std::string_view a, std::string_view b) {
        if (a.size() != b.size()) {
            return false;
        }
        for (std::size_t i = 0; i < a.size(); ++i) {
            char ca = a[i];
            char cb = b[i];
            if (ca >= 'a' && ca <= 'z') {
                ca = static_cast<char>(ca - 32);
            }
            if (cb >= 'a' && cb <= 'z') {
                cb = static_cast<char>(cb - 32);
            }
            if (ca != cb) {
                return false;
            }
        }
        return true;
    };
    for (std::size_t i = 0; i < kLevelNames.size(); ++i) {
        if (eqIgnoreCase(name, kLevelNames[i])) {
            return static_cast<Level>(i);
        }
    }
    return std::nullopt;
}
ConsoleSink::ConsoleSink(ConsoleSinkConfig cfg) noexcept
    : m_minLevel(cfg.minLevel), m_color(cfg.color && stdoutIsTty()) {
}

void ConsoleSink::write(Level lvl, std::string_view line) noexcept {
    if (!accepts(lvl)) {
        return;
    }
    int idx = static_cast<int>(lvl);
    if (idx < 0 || idx >= static_cast<int>(kLevelColors.size())) {
        idx = 0;
    }
    if (m_color) {
        const auto col = kLevelColors[static_cast<std::size_t>(idx)];
        std::fwrite(col.data(), 1, col.size(), stdout);
        std::fwrite(line.data(), 1, line.size(), stdout);
        std::fwrite(kReset.data(), 1, kReset.size(), stdout);
    } else {
        std::fwrite(line.data(), 1, line.size(), stdout);
    }
    std::fputc('\n', stdout);
    std::fflush(stdout);
}

void ConsoleSink::flush() noexcept {
    std::fflush(stdout);
}
// FileSink

FileSink::FileSink(FileSinkConfig cfg)
    : m_minLevel(cfg.minLevel), m_path(std::move(cfg.path)) {
    if (m_path.empty()) {
        return;
    }
    std::error_code ec;
    const std::filesystem::path p(m_path);
    if (p.has_parent_path()) {
        std::filesystem::create_directories(p.parent_path(), ec);
    }
    const auto mode = cfg.truncate ? (std::ios::out | std::ios::trunc)
                                   : (std::ios::out | std::ios::app);
    m_file.open(m_path, std::ios::binary | mode);
    m_open = static_cast<bool>(m_file);
    if (!m_open) {
        std::fprintf(stderr, "[Log] cannot open '%s'\n", m_path.c_str());
    }
}

FileSink::~FileSink() {
    if (m_file.is_open()) {
        m_file.flush();
        m_file.close();
    }
}

void FileSink::write(Level lvl, std::string_view line) noexcept {
    if (!accepts(lvl)) {
        return;
    }
    m_file.write(line.data(), static_cast<std::streamsize>(line.size()));
    m_file.put('\n');
    m_file.flush();
}

void FileSink::flush() noexcept {
    if (m_file.is_open()) {
        m_file.flush();
    }
}

// CompositeSink

CompositeSink::CompositeSink(std::vector<std::unique_ptr<ILogSink>> children)
    : m_children(std::move(children)) {
}

bool CompositeSink::accepts(Level lvl) const noexcept {
    for (const auto& c : m_children) {
        if (c && c->accepts(lvl)) {
            return true;
        }
    }
    return false;
}

void CompositeSink::write(Level lvl, std::string_view line) noexcept {
    for (auto& c : m_children) {
        if (c && c->accepts(lvl)) {
            c->write(lvl, line);
        }
    }
}

void CompositeSink::flush() noexcept {
    for (auto& c : m_children) {
        if (c) {
            c->flush();
        }
    }
}

std::unique_ptr<ILogSink> DefaultSinkFactory::makeNull() {
    return std::make_unique<NullSink>();
}

std::unique_ptr<ILogSink> DefaultSinkFactory::makeConsole(
    const ConsoleSinkConfig& cfg) {
    return std::make_unique<ConsoleSink>(cfg);
}

std::unique_ptr<ILogSink> DefaultSinkFactory::makeFile(
    const FileSinkConfig& cfg) {
    return std::make_unique<FileSink>(cfg);
}

std::unique_ptr<ILogSink> DefaultSinkFactory::makeComposite(
    std::vector<std::unique_ptr<ILogSink>> children) {
    return std::make_unique<CompositeSink>(std::move(children));
}

}  // namespace empire::log