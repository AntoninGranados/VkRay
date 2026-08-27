#pragma once

#include <string>
#include <vector>

std::string trim(const std::string& s);
std::vector<std::string> split(const std::string& s, char delim);
std::vector<float> parseNumbers(const std::string& expr);
