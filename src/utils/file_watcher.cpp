#include "file_watcher.hpp"

size_t FileWatcher::watch(const std::filesystem::path& path, std::function<void()> callback) {
    std::error_code ec;
    const size_t id = nextId++;
    watches.push_back({ id, path, std::filesystem::last_write_time(path, ec), std::move(callback) });
    return id;
}

void FileWatcher::unwatch(size_t id) {
    std::erase_if(watches, [id](const Watch& w) { return w.id == id; });
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
