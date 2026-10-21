#pragma once

#include "offline/job.hpp"

class Offline {
public:
    static int run();

private:
    Offline() = default;

    static void initParameters(const std::vector<ParameterOverride>& overrides);
};
