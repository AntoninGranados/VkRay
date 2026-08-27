#include "string_utils.hpp"

std::string trim(const std::string& s) {
    size_t begin = s.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(begin, end - begin + 1);
}

std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> tokens;
    size_t pos = 0;
    while (pos <= s.size()) {
        size_t next = s.find(delim, pos);
        tokens.push_back(trim(s.substr(pos, next == std::string::npos ? std::string::npos : next - pos)));
        if (next == std::string::npos) break;
        pos = next + 1;
    }
    return tokens;
}

std::vector<float> parseNumbers(const std::string& expr) {
    size_t open = expr.find('(');
    std::string inner = open == std::string::npos ? expr : expr.substr(open + 1, expr.rfind(')') - open - 1);
    std::vector<float> values;
    for (const std::string& token : split(inner, ',')) {
        if (token.empty()) continue;
        try {
            values.push_back(std::stof(token));
        } catch (...) {
        }
    }
    return values;
}
