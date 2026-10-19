#pragma once

#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace empire::log {

enum class Level : int {
    Trace = 0,
    Debug = 1,
    Info = 2,
    Warn = 3,
    Error = 4,
    Critical = 5,
    Off = 6,
};
const char* levelName(Level lvl) noexcept;
std::optional<Level> ParseLevel(std::string_view name) noexcept;

class ILogSink {
public:
    virtual ~ILogSink() = default;

    virtual bool accepts(Level lvl) const noexcept = 0;
    virtual void write(Level lvl, std::string_view line) noexcept = 0;
    virtual void flush() noexcept = 0;
};

class NullSink final : public ILogSink {
public:
    bool accepts(Level) const noexcept override {
        return false;
    }
    void write(Level, std::string_view) noexcept override {
    }
    void flush() noexcept override {
    }
};

// Console strategy
struct ConsoleSinkConfig {
    Level minLevel = Level::Info;
    bool color = true;
};

class ConsoleSink final : public ILogSink {
public:
    explicit ConsoleSink(ConsoleSinkConfig cfg) noexcept;

    bool accepts(Level lvl) const noexcept override {
        return lvl >= m_minLevel;
    }
    void write(Level lvl, std::string_view line) noexcept override;
    void flush() noexcept override;

private:
    Level m_minLevel;
    bool m_color;
};

// File strategy
struct FileSinkConfig {
    Level minLevel = Level::Debug;
    std::string path;
    bool truncate = false;
};

class FileSink final : public ILogSink {
public:
    explicit FileSink(FileSinkConfig cfg);
    ~FileSink() override;

    bool accepts(Level lvl) const noexcept override {
        return m_open && lvl >= m_minLevel;
    }
    void write(Level lvl, std::string_view line) noexcept override;
    void flush() noexcept override;

private:
    Level m_minLevel;
    std::string m_path;
    std::ofstream m_file;
    bool m_open = false;
};

// Composite: fan-out to N children, each with its own filter.
class CompositeSink final : public ILogSink {
public:
    explicit CompositeSink(std::vector<std::unique_ptr<ILogSink>> children);

    bool accepts(Level lvl) const noexcept override;
    void write(Level lvl, std::string_view line) noexcept override;
    void flush() noexcept override;

private:
    std::vector<std::unique_ptr<ILogSink>> m_children;
};

// Abstract factory. Every consumer of a sink goes through this. Tests can
// deriva a CapturingSinkFactory to produce in-memory sinks; platform layers can
// substitute platform-specific sinks
class ISinkFactory {
public:
    virtual ~ISinkFactory() = default;

    virtual std::unique_ptr<ILogSink> makeNull() = 0;
    virtual std::unique_ptr<ILogSink> makeConsole(const ConsoleSinkConfig&) = 0;
    virtual std::unique_ptr<ILogSink> makeFile(const FileSinkConfig&) = 0;
    virtual std::unique_ptr<ILogSink> makeComposite(
        std::vector<std::unique_ptr<ILogSink>>) = 0;
};

class DefaultSinkFactory final : public ISinkFactory {
public:
    std::unique_ptr<ILogSink> makeNull() override;
    std::unique_ptr<ILogSink> makeConsole(const ConsoleSinkConfig&) override;
    std::unique_ptr<ILogSink> makeFile(const FileSinkConfig&) override;
    std::unique_ptr<ILogSink> makeComposite(
        std::vector<std::unique_ptr<ILogSink>>) override;
};
}  // namespace empire::log
