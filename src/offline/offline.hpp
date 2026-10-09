#pragma once

#include "offline/job.hpp"

class Offline {
public:
    static void run();

private:
    Offline() = default;

    static void initParameters(const std::vector<ParameterOverride>& overrides);
};
