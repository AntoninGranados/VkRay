#pragma once

#include <string>
#include <unordered_map>

class GlslMangler {
public:
    struct MangleResult {
        bool ok = false;
        std::string error;
        std::string declarations;
        std::string body;
    };

    static std::string makePrefix(const std::string& type, int slot);
    static std::string mangleName(const std::string& prefix, const std::string& group, const std::string& name);

    static MangleResult mangle(const std::string& source, const std::string& prefix,
                                const std::unordered_map<std::string, std::string>& seedGlobals);
};
