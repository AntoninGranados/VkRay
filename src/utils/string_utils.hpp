#pragma once

#include <string>
#include <vector>

std::string trim(const std::string& s);
std::vector<std::string> split(const std::string& s, char delim);
std::vector<float> parseNumbers(const std::string& expr);
std::string snakeCaseToLabel(const std::string& id);
std::string camelCaseToLabel(const std::string& id);
std::string snakeCaseToPascalCase(const std::string& id);
