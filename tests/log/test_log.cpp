#include <empire/engine/log/LogSink.h>
#include <empire/engine/log/Logger.h>

#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace fs = std::filesystem;
using namespace empire::log;
using Catch::Matchers::ContainsSubstring;

// =========================================================================
// Test fixtures
// =========================================================================

namespace {

class CapturingSink : public ILogSink {
public:
    struct Entry {
        Level level;
        std::string line;
    };

    explicit CapturingSink(Level minLevel = Level::Trace) : m_min(minLevel) {
    }

    bool accepts(Level lvl) const noexcept override {
        ++m_acceptsCalls;
        return lvl >= m_min;
    }

    void write(Level lvl, std::string_view line) noexcept override {
        std::lock_guard g(m_mutex);
        m_entries.push_back({lvl, std::string(line)});
    }

    void flush() noexcept override {
        ++m_flushCount;
    }

    std::vector<Entry> entries() const {
        std::lock_guard g(m_mutex);
        return m_entries;
    }
    std::size_t entryCount() const {
        std::lock_guard g(m_mutex);
        return m_entries.size();
    }
    int acceptsCalls() const noexcept {
        return m_acceptsCalls.load();
    }
    int flushCount() const noexcept {
        return m_flushCount.load();
    }

private:
    Level m_min;
    mutable std::mutex m_mutex;
    std::vector<Entry> m_entries;
    mutable std::atomic<int> m_acceptsCalls{0};
    std::atomic<int> m_flushCount{0};
};

class ScopedTempDir {
public:
    ScopedTempDir() {
        std::error_code ec;
        auto base = fs::temp_directory_path(ec);
        if (ec) {
            base = fs::current_path(ec);
        }
        static std::atomic<unsigned> counter{0};
        path_ = base / ("empire_test_" + std::to_string(counter.fetch_add(1)));
        fs::remove_all(path_, ec);
        fs::create_directories(path_, ec);
    }
    ~ScopedTempDir() {
        std::error_code ec;
        fs::remove_all(path_, ec);
    }

    const fs::path& path() const noexcept {
        return path_;
    }

private:
    fs::path path_;
};

// Factory that lets us observe the actual sinks that buildLogger creates.
class TrackingFactory : public ISinkFactory {
public:
    CapturingSink* lastConsole = nullptr;
    CapturingSink* lastFile = nullptr;

    std::unique_ptr<ILogSink> makeNull() override {
        return std::make_unique<NullSink>();
    }
    std::unique_ptr<ILogSink> makeConsole(
        const ConsoleSinkConfig& cfg) override {
        auto s = std::make_unique<CapturingSink>(cfg.minLevel);
        lastConsole = s.get();
        return s;
    }
    std::unique_ptr<ILogSink> makeFile(const FileSinkConfig& cfg) override {
        auto s = std::make_unique<CapturingSink>(cfg.minLevel);
        lastFile = s.get();
        return s;
    }
    std::unique_ptr<ILogSink> makeComposite(
        std::vector<std::unique_ptr<ILogSink>> children) override {
        return std::make_unique<CompositeSink>(std::move(children));
    }
};

}  // namespace

// =========================================================================
// Level, ParseLevel, levelName
// =========================================================================

TEST_CASE("Level: levelName covers every enumerator", "[log][level]") {
    REQUIRE(std::string{levelName(Level::Trace)} == "TRACE");
    REQUIRE(std::string{levelName(Level::Debug)} == "DEBUG");
    REQUIRE(std::string{levelName(Level::Info)} == "INFO");
    REQUIRE(std::string{levelName(Level::Warn)} == "WARN");
    REQUIRE(std::string{levelName(Level::Error)} == "ERROR");
    REQUIRE(std::string{levelName(Level::Critical)} == "CRIT");
    REQUIRE(std::string{levelName(Level::Off)} == "OFF");
}

TEST_CASE("Level: levelName returns ? for out-of-range", "[log][level]") {
    REQUIRE(std::string{levelName(static_cast<Level>(-1))} == "?");
    REQUIRE(std::string{levelName(static_cast<Level>(99))} == "?");
}

TEST_CASE("ParseLevel: accepts canonical uppercase names", "[log][level]") {
    REQUIRE(ParseLevel("TRACE") == Level::Trace);
    REQUIRE(ParseLevel("DEBUG") == Level::Debug);
    REQUIRE(ParseLevel("INFO") == Level::Info);
    REQUIRE(ParseLevel("WARN") == Level::Warn);
    REQUIRE(ParseLevel("ERROR") == Level::Error);
    REQUIRE(ParseLevel("CRIT") == Level::Critical);
    REQUIRE(ParseLevel("OFF") == Level::Off);
}

TEST_CASE("ParseLevel: case insensitive", "[log][level]") {
    REQUIRE(ParseLevel("info") == Level::Info);
    REQUIRE(ParseLevel("Info") == Level::Info);
    REQUIRE(ParseLevel("iNfO") == Level::Info);
}

TEST_CASE("ParseLevel: unknown or empty returns nullopt", "[log][level]") {
    REQUIRE_FALSE(ParseLevel("").has_value());
    REQUIRE_FALSE(ParseLevel("info ").has_value());  // trailing space
    REQUIRE_FALSE(ParseLevel("WARNING").has_value());
    REQUIRE_FALSE(ParseLevel("!!").has_value());
}

// =========================================================================
// NullSink
// =========================================================================

TEST_CASE("NullSink: accepts nothing, write/flush are no-ops", "[log][sink]") {
    NullSink s;
    REQUIRE_FALSE(s.accepts(Level::Trace));
    REQUIRE_FALSE(s.accepts(Level::Critical));
    REQUIRE_FALSE(s.accepts(Level::Off));
    s.write(Level::Info, "ignored");
    s.flush();
    SUCCEED("NullSink survived");
}

// =========================================================================
// FileSink
// =========================================================================

TEST_CASE("FileSink: writes lines to disk", "[log][sink][file]") {
    ScopedTempDir tmp;
    const auto path = tmp.path() / "log.txt";

    FileSink sink({.minLevel = Level::Trace, .path = path.string()});
    REQUIRE(sink.accepts(Level::Info));

    sink.write(Level::Info, "line one");
    sink.write(Level::Warn, "line two");
    sink.flush();

    std::ifstream in(path);
    REQUIRE(in.is_open());
    std::string l1, l2;
    std::getline(in, l1);
    std::getline(in, l2);
    REQUIRE(l1 == "line one");
    REQUIRE(l2 == "line two");
}

TEST_CASE("FileSink: respects minLevel", "[log][sink][file]") {
    ScopedTempDir tmp;
    const auto path = tmp.path() / "log.txt";

    FileSink sink({.minLevel = Level::Warn, .path = path.string()});
    REQUIRE_FALSE(sink.accepts(Level::Info));
    REQUIRE(sink.accepts(Level::Warn));
    REQUIRE(sink.accepts(Level::Error));

    sink.write(Level::Info, "should not appear");
    sink.write(Level::Error, "should appear");
    sink.flush();

    std::ifstream in(path);
    std::string content((std::istreambuf_iterator<char>(in)),
                        std::istreambuf_iterator<char>());
    REQUIRE_THAT(content, !ContainsSubstring("should not appear"));
    REQUIRE_THAT(content, ContainsSubstring("should appear"));
}

TEST_CASE("FileSink: truncate=false appends", "[log][sink][file]") {
    ScopedTempDir tmp;
    const auto path = tmp.path() / "log.txt";

    {
        FileSink s({.minLevel = Level::Trace,
                    .path = path.string(),
                    .truncate = false});
        s.write(Level::Info, "first");
        s.flush();
    }
    {
        FileSink s({.minLevel = Level::Trace,
                    .path = path.string(),
                    .truncate = false});
        s.write(Level::Info, "second");
        s.flush();
    }

    std::ifstream in(path);
    std::string content((std::istreambuf_iterator<char>(in)),
                        std::istreambuf_iterator<char>());
    REQUIRE_THAT(content, ContainsSubstring("first"));
    REQUIRE_THAT(content, ContainsSubstring("second"));
}

TEST_CASE("FileSink: truncate=true overwrites", "[log][sink][file]") {
    ScopedTempDir tmp;
    const auto path = tmp.path() / "log.txt";

    {
        FileSink s({.minLevel = Level::Trace,
                    .path = path.string(),
                    .truncate = true});
        s.write(Level::Info, "first");
        s.flush();
    }
    {
        FileSink s({.minLevel = Level::Trace,
                    .path = path.string(),
                    .truncate = true});
        s.write(Level::Info, "second");
        s.flush();
    }

    std::ifstream in(path);
    std::string content((std::istreambuf_iterator<char>(in)),
                        std::istreambuf_iterator<char>());
    REQUIRE_THAT(content, !ContainsSubstring("first"));
    REQUIRE_THAT(content, ContainsSubstring("second"));
}

TEST_CASE("FileSink: empty path disables sink", "[log][sink][file]") {
    FileSink sink({.minLevel = Level::Trace, .path = ""});
    REQUIRE_FALSE(sink.accepts(Level::Info));
    REQUIRE_FALSE(sink.accepts(Level::Critical));
    sink.write(Level::Info, "ignored");
    sink.flush();
    SUCCEED("empty-path FileSink is inert");
}

// =========================================================================
// CompositeSink
// =========================================================================

TEST_CASE("CompositeSink: accepts if any child accepts",
          "[log][sink][composite]") {
    std::vector<std::unique_ptr<ILogSink>> children;
    children.push_back(std::make_unique<CapturingSink>(Level::Trace));
    children.push_back(std::make_unique<CapturingSink>(Level::Error));
    CompositeSink composite(std::move(children));

    REQUIRE(composite.accepts(Level::Info));
    REQUIRE(composite.accepts(Level::Error));
    REQUIRE(composite.accepts(Level::Off));
}

TEST_CASE("CompositeSink: rejects when all children reject",
          "[log][sink][composite]") {
    // Дети с minLevel = Off не принимают ничего, кроме Off (который
    // сообщением не бывает) — фактически sink выключен.
    std::vector<std::unique_ptr<ILogSink>> children;
    children.push_back(std::make_unique<CapturingSink>(Level::Off));
    children.push_back(std::make_unique<CapturingSink>(Level::Off));
    CompositeSink composite(std::move(children));

    REQUIRE_FALSE(composite.accepts(Level::Trace));
    REQUIRE_FALSE(composite.accepts(Level::Info));
    REQUIRE_FALSE(composite.accepts(Level::Error));
    REQUIRE_FALSE(composite.accepts(Level::Critical));
}

TEST_CASE("CompositeSink: fans out only to accepting children",
          "[log][sink][composite]") {
    auto aUp = std::make_unique<CapturingSink>(Level::Trace);
    auto bUp = std::make_unique<CapturingSink>(Level::Error);
    auto* a = aUp.get();
    auto* b = bUp.get();

    std::vector<std::unique_ptr<ILogSink>> children;
    children.push_back(std::move(aUp));
    children.push_back(std::move(bUp));
    CompositeSink composite(std::move(children));

    composite.write(Level::Info, "info");
    REQUIRE(a->entryCount() == 1u);
    REQUIRE(b->entryCount() == 0u);

    composite.write(Level::Error, "err");
    REQUIRE(a->entryCount() == 2u);
    REQUIRE(b->entryCount() == 1u);
}

TEST_CASE("CompositeSink: flush forwards to all children",
          "[log][sink][composite]") {
    auto aUp = std::make_unique<CapturingSink>();
    auto bUp = std::make_unique<CapturingSink>();
    auto* a = aUp.get();
    auto* b = bUp.get();

    std::vector<std::unique_ptr<ILogSink>> children;
    children.push_back(std::move(aUp));
    children.push_back(std::move(bUp));
    CompositeSink composite(std::move(children));

    composite.flush();
    REQUIRE(a->flushCount() == 1);
    REQUIRE(b->flushCount() == 1);
}

// =========================================================================
// Logger: channel registry
// =========================================================================

TEST_CASE("Logger: registerChannel records level", "[log][logger]") {
    auto sink = std::make_unique<CapturingSink>();
    Logger logger(std::move(sink));

    logger.registerChannel("svc", Level::Warn);
    REQUIRE(logger.channelLevel("svc") == Level::Warn);
}

TEST_CASE("Logger: registerChannel is idempotent", "[log][logger]") {
    auto sink = std::make_unique<CapturingSink>();
    Logger logger(std::move(sink));

    logger.registerChannel("svc", Level::Warn);
    logger.registerChannel("svc", Level::Trace);  // ignored
    REQUIRE(logger.channelLevel("svc") == Level::Warn);
}

TEST_CASE("Logger: registerChannel ignores empty name", "[log][logger]") {
    auto sink = std::make_unique<CapturingSink>();
    Logger logger(std::move(sink));

    logger.registerChannel("", Level::Debug);
    // Falls through to the default
    REQUIRE(logger.channelLevel("") == logger.defaultChannelLevel());
}

TEST_CASE("Logger: setChannelLevel overwrites existing", "[log][logger]") {
    auto sink = std::make_unique<CapturingSink>();
    Logger logger(std::move(sink));

    logger.registerChannel("svc", Level::Warn);
    logger.setChannelLevel("svc", Level::Debug);
    REQUIRE(logger.channelLevel("svc") == Level::Debug);
}

TEST_CASE("Logger: channelLevel falls back to default", "[log][logger]") {
    auto sink = std::make_unique<CapturingSink>();
    Logger logger(std::move(sink));

    logger.setDefaultChannelLevel(Level::Error);
    REQUIRE(logger.defaultChannelLevel() == Level::Error);
    REQUIRE(logger.channelLevel("unknown") == Level::Error);
}

TEST_CASE("Logger: isChannelEnabled respects threshold", "[log][logger]") {
    auto sink = std::make_unique<CapturingSink>();
    Logger logger(std::move(sink));

    logger.registerChannel("svc", Level::Warn);
    REQUIRE_FALSE(logger.isChannelEnabled("svc", Level::Info));
    REQUIRE(logger.isChannelEnabled("svc", Level::Warn));
    REQUIRE(logger.isChannelEnabled("svc", Level::Error));
}

// =========================================================================
// Logger: writing & filtering
// =========================================================================

TEST_CASE("Logger: writes when channel and sink both allow", "[log][logger]") {
    auto sinkUp = std::make_unique<CapturingSink>(Level::Trace);
    auto* sink = sinkUp.get();
    Logger logger(std::move(sinkUp));

    logger.registerChannel("svc", Level::Info);
    logger.log("svc", Level::Info, std::source_location::current(), "hello {}",
               42);

    const auto entries = sink->entries();
    REQUIRE(entries.size() == 1u);
    REQUIRE(entries[0].level == Level::Info);
    REQUIRE_THAT(entries[0].line, ContainsSubstring("hello 42"));
}

TEST_CASE("Logger: channel threshold gates before touching sink",
          "[log][logger]") {
    auto sinkUp = std::make_unique<CapturingSink>(Level::Trace);
    auto* sink = sinkUp.get();
    Logger logger(std::move(sinkUp));

    logger.registerChannel("svc", Level::Warn);
    logger.log("svc", Level::Info, std::source_location::current(), "dropped");

    REQUIRE(sink->entryCount() == 0u);
    // Filter hit the channel first — sink.accepts never called
    REQUIRE(sink->acceptsCalls() == 0);
}

TEST_CASE("Logger: sink threshold gates before formatting", "[log][logger]") {
    auto sinkUp = std::make_unique<CapturingSink>(Level::Error);
    auto* sink = sinkUp.get();
    Logger logger(std::move(sinkUp));

    logger.registerChannel("svc", Level::Trace);
    logger.log("svc", Level::Info, std::source_location::current(), "dropped");

    REQUIRE(sink->entryCount() == 0u);
    // accepts() called (so we know the sink was consulted)
    REQUIRE(sink->acceptsCalls() >= 1);
}

TEST_CASE("Logger: default channel overload", "[log][logger]") {
    auto sinkUp = std::make_unique<CapturingSink>(Level::Trace);
    auto* sink = sinkUp.get();
    Logger logger(std::move(sinkUp));

    logger.setDefaultChannelLevel(Level::Info);
    logger.log(Level::Warn, std::source_location::current(), "plain {}", 1);

    const auto entries = sink->entries();
    REQUIRE(entries.size() == 1u);
    REQUIRE_THAT(entries[0].line, ContainsSubstring("plain 1"));
}

// =========================================================================
// Logger: prefix formatting (indirect via sink)
// =========================================================================

TEST_CASE("Logger: prefix contains level name", "[log][logger][prefix]") {
    auto sinkUp = std::make_unique<CapturingSink>();
    auto* sink = sinkUp.get();
    LoggerFormat fmt{};
    fmt.timestamps = false;
    fmt.sourceLocation = false;
    Logger logger(std::move(sinkUp), fmt);
    logger.setDefaultChannelLevel(Level::Trace);

    logger.log(Level::Warn, std::source_location::current(), "msg");
    REQUIRE_THAT(sink->entries().at(0).line, ContainsSubstring("[WARN]"));
}

TEST_CASE("Logger: prefix includes channel when enabled",
          "[log][logger][prefix]") {
    auto sinkUp = std::make_unique<CapturingSink>();
    auto* sink = sinkUp.get();
    LoggerFormat fmt{};
    fmt.timestamps = false;
    fmt.sourceLocation = false;
    fmt.channel = true;
    Logger logger(std::move(sinkUp), fmt);
    logger.setDefaultChannelLevel(Level::Trace);

    logger.log("audio.bus", Level::Info, std::source_location::current(), "x");
    REQUIRE_THAT(sink->entries().at(0).line, ContainsSubstring("[audio.bus]"));
}

TEST_CASE("Logger: prefix excludes channel when disabled",
          "[log][logger][prefix]") {
    auto sinkUp = std::make_unique<CapturingSink>();
    auto* sink = sinkUp.get();
    LoggerFormat fmt{};
    fmt.timestamps = false;
    fmt.sourceLocation = false;
    fmt.channel = false;
    Logger logger(std::move(sinkUp), fmt);
    logger.setDefaultChannelLevel(Level::Trace);

    logger.log("audio.bus", Level::Info, std::source_location::current(), "x");
    REQUIRE_THAT(sink->entries().at(0).line, !ContainsSubstring("[audio.bus]"));
}

TEST_CASE("Logger: sourceLocation flag shows file:line",
          "[log][logger][prefix]") {
    auto sinkUp = std::make_unique<CapturingSink>();
    auto* sink = sinkUp.get();
    LoggerFormat fmt{};
    fmt.timestamps = false;
    fmt.sourceLocation = true;
    fmt.channel = false;
    Logger logger(std::move(sinkUp), fmt);
    logger.setDefaultChannelLevel(Level::Trace);

    logger.log(Level::Info, std::source_location::current(), "x");
    // Our own test file stem is present
    REQUIRE_THAT(sink->entries().at(0).line, ContainsSubstring("test_log"));
}

TEST_CASE("Logger: timestamps flag off removes bracket timestamp",
          "[log][logger][prefix]") {
    auto sinkUp = std::make_unique<CapturingSink>();
    auto* sink = sinkUp.get();
    LoggerFormat fmt{};
    fmt.timestamps = false;
    fmt.sourceLocation = false;
    fmt.channel = false;
    Logger logger(std::move(sinkUp), fmt);
    logger.setDefaultChannelLevel(Level::Trace);

    logger.log(Level::Info, std::source_location::current(), "x");
    const std::string line = sink->entries().at(0).line;
    // Prefix should be exactly "[INFO] "
    REQUIRE_THAT(line, ContainsSubstring("[INFO] "));
    REQUIRE(line.find(':') == std::string::npos);  // no hh:mm:ss
}

// =========================================================================
// Logger: flush
// =========================================================================

TEST_CASE("Logger: flush calls sink->flush", "[log][logger]") {
    auto sinkUp = std::make_unique<CapturingSink>();
    auto* sink = sinkUp.get();
    Logger logger(std::move(sinkUp));

    logger.flush();
    REQUIRE(sink->flushCount() == 1);
}

// =========================================================================
// ScopedLogger / defaultLogger
// =========================================================================

TEST_CASE("ScopedLogger: installs and restores default logger",
          "[log][logger][default]") {
    Logger* prev = defaultLogger();

    auto sinkUp = std::make_unique<CapturingSink>();
    Logger logger(std::move(sinkUp));

    {
        ScopedLogger scoped(&logger);
        REQUIRE(defaultLogger() == &logger);
    }
    REQUIRE(defaultLogger() == prev);
}

TEST_CASE("ScopedLogger: nested scopes restore in LIFO order",
          "[log][logger][default]") {
    auto sinkA = std::make_unique<CapturingSink>();
    auto sinkB = std::make_unique<CapturingSink>();
    Logger a(std::move(sinkA));
    Logger b(std::move(sinkB));
    Logger* prev = defaultLogger();

    {
        ScopedLogger outer(&a);
        REQUIRE(defaultLogger() == &a);
        {
            ScopedLogger inner(&b);
            REQUIRE(defaultLogger() == &b);
        }
        REQUIRE(defaultLogger() == &a);
    }
    REQUIRE(defaultLogger() == prev);
}

TEST_CASE("Logger macros route through defaultLogger", "[log][logger][macro]") {
    auto sinkUp = std::make_unique<CapturingSink>(Level::Trace);
    auto* sink = sinkUp.get();
    Logger logger(std::move(sinkUp));
    logger.setDefaultChannelLevel(Level::Trace);

    ScopedLogger scoped(&logger);
    GAME_LOG_CH_INFO("test.ch", "value={}", 123);

    const auto entries = sink->entries();
    REQUIRE(entries.size() == 1u);
    REQUIRE_THAT(entries[0].line, ContainsSubstring("value=123"));
}

TEST_CASE("Logger macros are no-op without defaultLogger",
          "[log][logger][macro]") {
    // Ensure no ambient logger is installed for this test.
    ScopedLogger noop(nullptr);
    // Should not crash, should not write anywhere.
    GAME_LOG_CH_INFO("test.ch", "ignored {}", 1);
    SUCCEED("macro without logger is a no-op");
}

// =========================================================================
// buildLogger
// =========================================================================

TEST_CASE("buildLogger: applies defaultChannelLevel", "[log][build]") {
    TrackingFactory factory;
    LoggerConfig cfg{};
    cfg.consoleEnabled = false;
    cfg.fileEnabled = false;
    cfg.defaultChannelLevel = Level::Warn;

    auto logger = buildLogger(factory, cfg);
    REQUIRE(logger != nullptr);
    REQUIRE(logger->defaultChannelLevel() == Level::Warn);
}

TEST_CASE("buildLogger: applies per-channel levels", "[log][build]") {
    TrackingFactory factory;
    LoggerConfig cfg{};
    cfg.consoleEnabled = false;
    cfg.fileEnabled = false;
    cfg.channelLevels = {
        {"audio", Level::Debug},
        {"ai", Level::Critical},
    };

    auto logger = buildLogger(factory, cfg);
    REQUIRE(logger->channelLevel("audio") == Level::Debug);
    REQUIRE(logger->channelLevel("ai") == Level::Critical);
}

TEST_CASE("buildLogger: console-only config uses console sink",
          "[log][build]") {
    TrackingFactory factory;
    LoggerConfig cfg{};
    cfg.consoleEnabled = true;
    cfg.consoleLevel = Level::Info;
    cfg.fileEnabled = false;
    cfg.defaultChannelLevel = Level::Trace;

    auto logger = buildLogger(factory, cfg);
    REQUIRE(factory.lastConsole != nullptr);
    REQUIRE(factory.lastFile == nullptr);

    logger->log(Level::Info, std::source_location::current(), "hi");
    REQUIRE(factory.lastConsole->entryCount() == 1u);
}

TEST_CASE("buildLogger: file-only config uses file sink", "[log][build]") {
    TrackingFactory factory;
    LoggerConfig cfg{};
    cfg.consoleEnabled = false;
    cfg.fileEnabled = true;
    cfg.fileLevel = Level::Debug;
    cfg.logFilePath = "/tmp/x.log";
    cfg.defaultChannelLevel = Level::Trace;

    auto logger = buildLogger(factory, cfg);
    REQUIRE(factory.lastConsole == nullptr);
    REQUIRE(factory.lastFile != nullptr);
}

TEST_CASE("buildLogger: file config without path falls back to null",
          "[log][build]") {
    TrackingFactory factory;
    LoggerConfig cfg{};
    cfg.consoleEnabled = false;
    cfg.fileEnabled = true;
    cfg.logFilePath = "";  // missing
    cfg.defaultChannelLevel = Level::Trace;

    auto logger = buildLogger(factory, cfg);
    REQUIRE(factory.lastConsole == nullptr);
    REQUIRE(factory.lastFile == nullptr);

    // NullSink accepts nothing
    logger->log(Level::Info, std::source_location::current(), "ignored");
    SUCCEED("logging into a null logger is safe");
}