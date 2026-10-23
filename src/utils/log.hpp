#pragma once

#include <filesystem>
#include <fstream>
#include <functional>
#include <string>
#include <string_view>

enum class LogLevel { Debug, Info, Success, Warn, Error };

struct LogEntry {
    LogLevel level;
    std::string source;
    std::string message;
};

class Log {
public:
    static void setConsumer(std::function<void(const LogEntry&)> fn);

    static void info(std::string_view msg);
    static void success(std::string_view msg);
    static void warn(std::string_view msg);
    static void error(std::string_view msg);
    static void debug(std::string_view msg);

    static void info(std::string_view source, std::string_view msg);
    static void success(std::string_view source, std::string_view msg);
    static void warn(std::string_view source, std::string_view msg);
    static void error(std::string_view source, std::string_view msg);
    static void debug(std::string_view source, std::string_view msg);

private:
    Log() = default;
    ~Log() = default;

    static Log& get();
    static std::filesystem::path getLogDirectory();
    void push(LogLevel level, std::string_view source, std::string_view msg);
    void ensureFile();

    std::function<void(const LogEntry&)> consumer;
    std::ofstream file;
};
