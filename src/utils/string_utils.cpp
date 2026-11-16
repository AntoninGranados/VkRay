#include "string_utils.hpp"

#include <cctype>

std::string snakeCaseToLabel(const std::string& id) {
    std::string result;
    bool capitalize = true;
    for (const char c : id) {
        if (capitalize) {
            result += std::toupper(c);
            capitalize = false;
        } else if (c == '_') {
            result += ' ';
            capitalize = true;
        } else {
            result += c;
        }
    }
    return result;
}

std::string camelCaseToLabel(const std::string& id) {
    std::string result;
    for (size_t i = 0; i < id.size(); i++) {
        if (i > 0 && std::isupper(id[i])) result += ' ';
        result += i == 0 ? static_cast<char>(std::toupper(id[i])) : id[i];
    }
    return result;
}
