#pragma once

#include "offline/job.hpp"
#include "offline/job_queue.hpp"

class Offline {
public:
    static void run(JobQueue& queue);

private:
    Offline() = default;

    static void initParameters(const std::vector<ParameterOverride>& overrides);
};
