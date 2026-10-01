#include "log.hpp"

#include <cctype>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <print>

namespace {
std::string stripAnsiCodes(std::string_view s) {
    std::string result;
    result.reserve(s.size());
    size_t i = 0;
    while (i < s.size()) {
        if (s[i] == '\033' && i + 1 < s.size() && s[i + 1] == '[') {
            size_t j = i + 2;
            while (j < s.size() && !std::isalpha(static_cast<unsigned char>(s[j]))) j++;
            i = j < s.size() ? j + 1 : j;
            continue;
        }
        result += s[i];
        i++;
    }
    return result;
}
} // namespace

static std::string_view prefix(LogLevel l) {
    switch (l) {
    case LogLevel::Info:
        return "[INFO]";
    case LogLevel::Success:
        return "[SUCCESS]";
    case LogLevel::Warn:
        return "[WARN]";
    case LogLevel::Error:
        return "[ERROR]";
    case LogLevel::Debug:
        return "[DEBUG]";
    }
    std::unreachable();
}

Log& Log::get() {
    static Log instance;
    return instance;
}

void Log::setConsumer(std::function<void(const LogEntry&)> fn) { get().consumer = std::move(fn); }

void Log::setLevel(LogLevel level) { get().minLevel = level; }

void Log::info(std::string_view msg) { get().push(LogLevel::Info, {}, msg); }
void Log::success(std::string_view msg) { get().push(LogLevel::Success, {}, msg); }
void Log::warn(std::string_view msg) { get().push(LogLevel::Warn, {}, msg); }
void Log::error(std::string_view msg) { get().push(LogLevel::Error, {}, msg); }
void Log::debug(std::string_view msg) { get().push(LogLevel::Debug, {}, msg); }

void Log::info(std::string_view src, std::string_view msg) { get().push(LogLevel::Info, src, msg); }
void Log::success(std::string_view src, std::string_view msg) { get().push(LogLevel::Success, src, msg); }
void Log::warn(std::string_view src, std::string_view msg) { get().push(LogLevel::Warn, src, msg); }
void Log::error(std::string_view src, std::string_view msg) { get().push(LogLevel::Error, src, msg); }
void Log::debug(std::string_view src, std::string_view msg) { get().push(LogLevel::Debug, src, msg); }

void Log::ensureFile() {
    if (file.is_open()) return;
    std::filesystem::create_directories("logs");
    file.open("logs/vkray.log", std::ios::app);
    auto now = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
    std::println(file, "\n=== Session {} ===", std::format("{:%Y-%m-%d %H:%M:%S}", now));
}

void Log::push(LogLevel level, std::string_view source, std::string_view msg) {
    if (level < minLevel) return;
    while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r')) msg.remove_suffix(1);
    const std::string plain = stripAnsiCodes(msg);
    entries.push_back({level, std::string(source), plain});
    const LogEntry& entry = entries.back();

    auto& out = (level == LogLevel::Error || level == LogLevel::Warn) ? std::cerr : std::cout;
    if (source.empty())
        std::println(out, "{} {}", prefix(level), msg);
    else
        std::println(out, "{} [{}] {}", prefix(level), source, msg);

    ensureFile();
    if (source.empty())
        std::println(file, "{} {}", prefix(level), plain);
    else
        std::println(file, "{} [{}] {}", prefix(level), source, plain);

    if (consumer) consumer(entry);
}
