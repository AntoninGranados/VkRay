#include "file_watcher.hpp"

void FileWatcher::watch(const std::filesystem::path& path, std::function<void()> callback) {
    std::error_code ec;
    watches.push_back({ path, std::filesystem::last_write_time(path, ec), std::move(callback) });
}

void FileWatcher::poll() {
    for (Watch& watch : watches) {
        std::error_code ec;
        std::filesystem::file_time_type writeTime = std::filesystem::last_write_time(watch.path, ec);
        if (ec || writeTime == watch.writeTime) continue;
        watch.writeTime = writeTime;
        watch.callback();
    }
}
