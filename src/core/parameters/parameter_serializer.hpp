#pragma once

#include <filesystem>

#include "core/parameters/parameters.hpp"

namespace ParameterSerializer {
    void saveDocumentation(std::filesystem::path path);
    ParameterRegistry load(std::filesystem::path path);
}
