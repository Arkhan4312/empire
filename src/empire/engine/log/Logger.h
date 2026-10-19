#pragma once
#include <empire/engine/log/LogSink.h>

#include <atomic>
#include <format>
#include <map>
#include <memory>
#include <mutex>
#include <source_location>
#include <string>
#include <string_view>
#include <utility>

namespace empire::log {
using Channel = std::string_view;

struct LoggerFormat {
    bool timestamps = true;
    bool threadId = false;
    bool sourceLocation = true;
    bool functionName = false;
    bool channel = true;
};

struct LoggerConfig {
    bool consoleEnabled = true;
    Level consoleLevel = Level::Info;
    bool colorConsole = true;

    bool fileEnabled = false;
    Level fileLevel = Level::Debug;
    std::string logFilePath;
    bool truncate = false;

    LoggerFormat format{};

    std::map<std::string, Level, std::less<>> channelLevels;
    Level defaultChannelLevel = Level::Info;
};

class Logger {
public:
    static constexpr Channel kDefaultChannel = "default";

    explicit Logger(std::unique_ptr<ILogSink> sink, LoggerFormat fmt = {});
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    void setSink(std::unique_ptr<ILogSink> sink);
    void flush() noexcept;

    void registerChannel(Channel ch, Level minLevel = Level::Info);
    void setChannelLevel(Channel ch, Level minLevel);
    Level channelLevel(Channel ch) const noexcept;

    void setDefaultChannelLevel(Level lvl) noexcept {
        m_defaultChannelLevel = lvl;
    }
    Level defaultChannelLevel() const noexcept {
        return m_defaultChannelLevel;
    }

    bool isChannelEnabled(Channel ch, Level lvl) const noexcept;

    template <typename... Args>
    void log(Channel ch, Level lvl, const std::source_location& loc,
             std::format_string<Args...> fmt, Args&&... args) {
        auto sink = m_sink.load(std::memory_order_acquire);
        if (!sink) {
            return;
        }
        if (!isChannelEnabled(ch, lvl)) {
            return;
        }
        if (!sink->accepts(lvl)) {
            return;
        }

        std::string msg;
        try {
            msg = std::format(fmt, std::forward<Args>(args)...);
        } catch (const std::format_error&) {
            msg = "<format error>";
        }
        std::string line = formatPrefix(ch, lvl, loc);
        line += msg;

        std::lock_guard lock(m_writeMutex);
        sink->write(lvl, line);
    }

    template <typename... Args>
    void log(Level lvl, const std::source_location& loc,
             std::format_string<Args...> fmt, Args&&... args) {
        log(kDefaultChannel, lvl, loc, fmt, std::forward<Args>(args)...);
    }

private:
    std::string formatPrefix(Channel ch, Level lvl,
                             const std::source_location& loc) const;
    std::atomic<std::shared_ptr<ILogSink>> m_sink;
    std::mutex m_writeMutex;
    LoggerFormat m_format;

    std::map<std::string, Level, std::less<>> m_channelLevels;
    Level m_defaultChannelLevel = Level::Info;
};
// Build a concrete Logger using a factory + high-level config. This is the
// only function that needs to know how LoggerConfig maps onto sinks, so the
// mapping lives in one place
std::unique_ptr<Logger> buildLogger(ISinkFactory& factory,
                                    const LoggerConfig& cfg);

// Ambient logger. nullptr is a valid state.
Logger* defaultLogger() noexcept;
void setDefaultLogger(Logger* logger) noexcept;

class ScopedLogger {
public:
    explicit ScopedLogger(Logger* replacement) noexcept
        : m_prev(defaultLogger()) {
        setDefaultLogger(replacement);
    }
    ~ScopedLogger() {
        setDefaultLogger(m_prev);
    }
    ScopedLogger(const ScopedLogger&) = delete;
    ScopedLogger& operator=(const ScopedLogger&) = delete;

private:
    Logger* m_prev;
};
}  // namespace empire::log

#if defined(NDEBUG)
#define GAME_LOG_HAS_TRACE 0
#define GAME_LOG_HAS_DEBUG 0
#else
#define GAME_LOG_HAS_TRACE 1
#define GAME_LOG_HAS_DEBUG 1
#endif

#define GAME_LOG_CH_AT(channel_, lvl, fmt, ...)                           \
    do {                                                                  \
        if (auto* empire_log_ambient_ = ::empire::log::defaultLogger()) { \
            empire_log_ambient_->log(channel_, lvl,                       \
                                     ::std::source_location::current(),   \
                                     fmt __VA_OPT__(, ) __VA_ARGS__);     \
        }                                                                 \
    } while (0)

#define GAME_LOG_CH_TRACE(ch, fmt, ...)             \
    GAME_LOG_CH_AT(ch, ::empire::log::Level::Trace, \
                   fmt __VA_OPT__(, ) __VA_ARGS__)
#define GAME_LOG_CH_DEBUG(ch, fmt, ...)             \
    GAME_LOG_CH_AT(ch, ::empire::log::Level::Debug, \
                   fmt __VA_OPT__(, ) __VA_ARGS__)
#define GAME_LOG_CH_INFO(ch, fmt, ...)             \
    GAME_LOG_CH_AT(ch, ::empire::log::Level::Info, \
                   fmt __VA_OPT__(, ) __VA_ARGS__)
#define GAME_LOG_CH_WARN(ch, fmt, ...)             \
    GAME_LOG_CH_AT(ch, ::empire::log::Level::Warn, \
                   fmt __VA_OPT__(, ) __VA_ARGS__)
#define GAME_LOG_CH_ERROR(ch, fmt, ...)             \
    GAME_LOG_CH_AT(ch, ::empire::log::Level::Error, \
                   fmt __VA_OPT__(, ) __VA_ARGS__)
#define GAME_LOG_CH_CRITICAL(ch, fmt, ...)             \
    GAME_LOG_CH_AT(ch, ::empire::log::Level::Critical, \
                   fmt __VA_OPT__(, ) __VA_ARGS__)

// Legacy macros — channel "default"
#if GAME_LOG_HAS_TRACE
#define GAME_LOG_TRACE(fmt, ...)                              \
    GAME_LOG_CH_TRACE(::empire::log::Logger::kDefaultChannel, \
                      fmt __VA_OPT__(, ) __VA_ARGS__)
#else
#define GAME_LOG_TRACE(fmt, ...) ((void)0)
#endif

#if GAME_LOG_HAS_DEBUG
#define GAME_LOG_DEBUG(fmt, ...)                              \
    GAME_LOG_CH_DEBUG(::empire::log::Logger::kDefaultChannel, \
                      fmt __VA_OPT__(, ) __VA_ARGS__)
#else
#define GAME_LOG_DEBUG(fmt, ...) ((void)0)
#endif

#define GAME_LOG_INFO(fmt, ...)                              \
    GAME_LOG_CH_INFO(::empire::log::Logger::kDefaultChannel, \
                     fmt __VA_OPT__(, ) __VA_ARGS__)
#define GAME_LOG_WARN(fmt, ...)                              \
    GAME_LOG_CH_WARN(::empire::log::Logger::kDefaultChannel, \
                     fmt __VA_OPT__(, ) __VA_ARGS__)
#define GAME_LOG_ERROR(fmt, ...)                              \
    GAME_LOG_CH_ERROR(::empire::log::Logger::kDefaultChannel, \
                      fmt __VA_OPT__(, ) __VA_ARGS__)
#define GAME_LOG_CRITICAL(fmt, ...)                              \
    GAME_LOG_CH_CRITICAL(::empire::log::Logger::kDefaultChannel, \
                         fmt __VA_OPT__(, ) __VA_ARGS__)