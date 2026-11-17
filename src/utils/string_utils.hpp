#pragma once

#include <string>
#include <string_view>

std::string snakeCaseToLabel(const std::string& id);
std::string camelCaseToLabel(const std::string& id);
bool containsIgnoreCase(std::string_view text, std::string_view query);
