#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

namespace aperture {

constexpr int kSize = 256;

void makeCircle(std::vector<uint8_t>& out);
void makePolygon(std::vector<uint8_t>& out, int blades, float rotationDeg);
bool loadFromFile(std::vector<uint8_t>& out, const std::filesystem::path& path);

} // namespace aperture
