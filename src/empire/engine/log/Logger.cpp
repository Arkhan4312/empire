#include <empire/engine/log/Logger.h>

#include <array>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <iterator>
#include <thread>

namespace empire::log {
namespace {
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
#if defined(_WIN32)
    localtime_s(&tmv, &secs);
#else
    localtime_r(&secs, &tmv);
#endif

    std::format_to(std::back_inserter(out), "{:02}:{:02}:{:02}.{:03}",
                   tmv.tm_hour, tmv.tm_min, tmv.tm_sec, static_cast<int>(ms));
}
// plain POD
std::atomic<Logger*> g_defaultLogger{nullptr};
}  // namespace

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

void Logger::registerChannel(Channel ch, Level minLevel) {
    if (ch.empty()) {
        return;
    }
    auto it = m_channelLevels.find(ch);
    if (it != m_channelLevels.end()) {
        return;
    }
    m_channelLevels.emplace(std::string(ch), minLevel);
}

void Logger::setChannelLevel(Channel ch, Level minLevel) {
    if (ch.empty()) {
        return;
    }
    m_channelLevels.insert_or_assign(std::string(ch), minLevel);
}

Level Logger::channelLevel(Channel ch) const noexcept {
    auto it = m_channelLevels.find(ch);
    return it == m_channelLevels.end() ? m_defaultChannelLevel : it->second;
}

bool Logger::isChannelEnabled(Channel ch, Level lvl) const noexcept {
    return lvl >= channelLevel(ch);
}

std::string Logger::formatPrefix(Channel ch, Level lvl,
                                 const std::source_location& loc) const {
    std::string line;
    line.reserve(128);

    if (m_format.timestamps) {
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
    const char* lvlName = (idx >= 0 && idx < 7) ? levelName(lvl) : "?";
    line += '[';
    line += lvlName;
    line += ']';
    if (m_format.channel && !ch.empty()) {
        line += '[';
        line += ch;
        line += ']';
    }
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

    auto logger = std::make_unique<Logger>(std::move(sink), cfg.format);
    logger->setDefaultChannelLevel(cfg.defaultChannelLevel);
    for (const auto& [name, lvl] : cfg.channelLevels) {
        logger->setChannelLevel(name, lvl);
    }
    return logger;
}

Logger* defaultLogger() noexcept {
    return g_defaultLogger.load(std::memory_order_acquire);
}

void setDefaultLogger(Logger* logger) noexcept {
    g_defaultLogger.store(logger, std::memory_order_release);
}
}  // namespace empire::log