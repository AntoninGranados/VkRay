#pragma once

#include <filesystem>

#include "job.hpp"

class JobQueue {
public:
    static JobQueue fromFile(const std::filesystem::path& path);

    void enqueue(Job job);

    void cancel(size_t index);
    void clearPending();

    bool isEmpty()   const;
    bool isRunning() const;

    const std::vector<Job>& entries() const;

    Job*       nextPending();
    const Job* running() const;

    void setProgress(float progress);
    void complete();
    void fail();

private:
    std::vector<Job> jobs;
    int              runningIndex = -1;
};
