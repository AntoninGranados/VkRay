#pragma once

#include <fstream>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

enum class LogLevel { Debug, Info, Success, Warn, Error };

struct LogEntry {
    LogLevel    level;
    std::string source;
    std::string message;
};

class Log {
public:
    static void setConsumer(std::function<void(const LogEntry&)> fn);
    static void setLevel(LogLevel level);

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
    Log()  = default;
    ~Log() = default;

    static Log& get();
    void        push(LogLevel level, std::string_view source, std::string_view msg);
    void        ensureFile();

    std::vector<LogEntry>                entries;
    std::function<void(const LogEntry&)> consumer;
    std::ofstream                        file;
    LogLevel                             minLevel = LogLevel::Debug;
};
