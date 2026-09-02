#pragma once

#include <filesystem>
#include <functional>
#include <vector>

class FileWatcher {
public:
    void watch(const std::filesystem::path& path, std::function<void()> callback);
    void poll();

private:
    struct Watch {
        std::filesystem::path path;
        std::filesystem::file_time_type writeTime;
        std::function<void()> callback;
    };

    std::vector<Watch> watches;
};
