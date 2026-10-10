#pragma once

#include <filesystem>

#include "core/fields/parameters.hpp"

namespace ParameterSerializer {
void saveDocumentation(const ParameterRegistry& parameters, std::filesystem::path path);
ParameterRegistry load(std::filesystem::path path);
} // namespace ParameterSerializer
