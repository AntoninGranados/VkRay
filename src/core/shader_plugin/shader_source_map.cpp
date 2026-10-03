#include "shader_source_map.hpp"

#include <algorithm>
#include <format>
#include <optional>
#include <regex>
#include <sstream>
#include <string_view>
#include <utility>
#include <vector>

#include "utils/resources.hpp"

namespace {
const std::string kMarkerPrefix = "// @src ";

std::optional<std::pair<std::string, int>> parseMarker(const std::string& line) {
    const size_t pos = line.rfind(kMarkerPrefix);
    if (pos == std::string::npos) return std::nullopt;

    const std::string rest = line.substr(pos + kMarkerPrefix.size());
    const size_t colon = rest.rfind(':');
    if (colon == std::string::npos) return std::nullopt;

    try {
        return std::make_pair(rest.substr(0, colon), std::stoi(rest.substr(colon + 1)));
    } catch (...) { return std::nullopt; }
}

std::optional<std::pair<std::string, int>> resolveOrigin(const std::filesystem::path& generatedFile, int line) {
    const std::optional<std::string_view> source = Resources::find(generatedFile);
    if (!source) return std::nullopt;

    std::vector<std::string> lines;
    std::istringstream stream{std::string(*source)};
    std::string current;
    while (std::getline(stream, current)) lines.push_back(current);

    for (int i = std::min(line, static_cast<int>(lines.size())) - 1; i >= 0; i--) {
        std::optional<std::pair<std::string, int>> origin = parseMarker(lines[i]);
        if (origin) return origin;
    }
    return std::nullopt;
}
} // namespace

std::string ShaderSourceMap::marker(const std::filesystem::path& path, int line) {
    return std::format("  {}{}:{}", kMarkerPrefix, path.string(), line);
}

std::string ShaderSourceMap::remapError(const std::string& message) {
    static const std::regex pattern(R"((\S+\.glsl):(\d+):)");

    std::string result;
    size_t lastPos = 0;
    for (auto it = std::sregex_iterator(message.begin(), message.end(), pattern); it != std::sregex_iterator(); ++it) {
        const std::smatch& match = *it;
        result += message.substr(lastPos, match.position() - lastPos);

        const std::optional<std::pair<std::string, int>> origin =
            resolveOrigin(match[1].str(), std::stoi(match[2].str()));
        result += origin ? std::format("{}:{}:", origin->first, origin->second) : match.str();

        lastPos = match.position() + match.length();
    }
    result += message.substr(lastPos);
    return result;
}
