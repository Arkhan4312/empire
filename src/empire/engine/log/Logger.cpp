#include <empire/engine/log/Logger.h>

#include <array>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <iterator>
#include <thread>

namespace game::log {
namespace {
constexpr std::array<std::string_view, 7> kLevelNames{
    "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "CRIT", "OFF",
};
std::string_view fileStem(std::string_view path) noexcept {
    const auto slash = path.find_last_of("/\\");
    std::string_view name =
        (slash == std::string_view::npos) ? path : path.substr(slash + 1);
    const auto dot = name.find_last_of('.');
    if (dot != std::string_view::npos) {
        name = name.substr(0, dot);
    }
    return name;
}

void appendTimestamp(std::string& out) {
    using clock = std::chrono::system_clock;
    const auto now = clock::now();
    const auto secs = clock::to_time_t(now);
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        now.time_since_epoch())
                        .count() %
                    1000;
    std::tm tmv{};
    localtime_s(&tmv, &secs);
    std::format_to(std::back_inserter(out), "{:02}:{:02}:{:02}.{:03}",
                   tmv.tm_hour, tmv.tm_min, tmv.tm_sec, static_cast<int>(ms));
}
// plain POD
Logger* g_defaultLogger = nullptr;
}  // namespace

const char* levelName(Level lvl) noexcept {
    const int idx = static_cast<int>(lvl);
    if (idx < 0 || idx >= static_cast<int>(kLevelNames.size())) {
        return "?";
    }
    return kLevelNames[static_cast<std::size_t>(idx)].data();
}
Logger::Logger(std::unique_ptr<ILogSink> sink, LoggerFormat fmt)
    : m_sink(std::shared_ptr<ILogSink>{std::move(sink)}), m_format(fmt) {
}

Logger::~Logger() {
    flush();
}

void Logger::setSink(std::unique_ptr<ILogSink> sink) {
    m_sink.store(std::shared_ptr<ILogSink>{std::move(sink)},
                 std::memory_order_release);
}

void Logger::flush() noexcept {
    auto sink = m_sink.load(std::memory_order_acquire);
    if (!sink) {
        return;
    }
    std::lock_guard lock(m_writeMutex);
    sink->flush();
}

std::string Logger::formatPrefix(Level lvl,
                                 const std::source_location& loc) const {
    std::string line;
    line.reserve(96);

    if (m_format.timestams) {
        line += '[';
        appendTimestamp(line);
        line += "] ";
    }
    if (m_format.threadId) {
        std::ostringstream oss;
        oss << std::this_thread::get_id();
        line += '[';
        line += oss.str();
        line += "] ";
    }

    int idx = static_cast<int>(lvl);
    if (idx < 0 || idx >= static_cast<int>(kLevelNames.size())) {
        idx = 0;
    }
    line += '[';
    line += kLevelNames[static_cast<std::size_t>(idx)];
    line += ']';

    if (m_format.sourceLocation) {
        line += '[';
        line += fileStem(loc.file_name());
        line += ':';
        std::format_to(std::back_inserter(line), "{}", loc.line());
        line += ']';
    }
    if (m_format.functionName) {
        line += '[';
        line += loc.function_name();
        line += ']';
    }
    line += ' ';
    return line;
}

std::unique_ptr<Logger> buildLogger(ISinkFactory& factory,
                                    const LoggerConfig& cfg) {
    std::vector<std::unique_ptr<ILogSink>> children;

    if (cfg.consoleEnabled) {
        children.push_back(factory.makeConsole(
            {.minLevel = cfg.consoleLevel, .color = cfg.colorConsole}));
    }
    if (cfg.fileEnabled && !cfg.logFilePath.empty()) {
        children.push_back(factory.makeFile({
            .minLevel = cfg.fileLevel,
            .path = cfg.logFilePath,
            .truncate = cfg.truncate,
        }));
    }
    std::unique_ptr<ILogSink> sink;
    if (children.empty()) {
        sink = factory.makeNull();
    } else if (children.size() == 1) {
        sink = std::move(children.front());
    } else {
        sink = factory.makeComposite(std::move(children));
    }
    return std::make_unique<Logger>(std::move(sink), cfg.format);
}

Logger* defaultLogger() noexcept {
    return g_defaultLogger;
}

void setDefaultLogger(Logger* logger) noexcept {
    g_defaultLogger = logger;
}
}  // namespace game::log