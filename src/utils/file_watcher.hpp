#pragma once

#include <cstddef>
#include <filesystem>
#include <functional>
#include <vector>

class FileWatcher {
public:
    size_t watch(const std::filesystem::path& path, std::function<void()> callback);
    void unwatch(size_t id);
    void poll();

private:
    struct Watch {
        size_t id;
        std::filesystem::path path;
        std::filesystem::file_time_type writeTime;
        std::function<void()> callback;
    };

    std::vector<Watch> watches;
    size_t nextId = 1;
};
