#include "aperture.hpp"

#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include "stb_image/stb_image.h"

#include "utils/log.hpp"

namespace aperture {

namespace {

constexpr float kFeather = 2.0f / kSize;

float circleCoverage(float x, float y) {
    return glm::clamp((1.0f - std::sqrt(x * x + y * y)) / kFeather, 0.0f, 1.0f);
}

float polygonCoverage(float x, float y, int blades, float rotationRad) {
    const float r         = std::sqrt(x * x + y * y);
    const float invBlades = 1.0f / static_cast<float>(blades);
    const float sector    = glm::two_pi<float>() * invBlades;
    float theta = std::atan2(y, x) - rotationRad;
    theta = std::fmod(theta, sector);
    if (theta <= 0) theta += sector;
    const float rMax = std::cos(glm::pi<float>() * invBlades) / std::cos(theta - glm::pi<float>() * invBlades);
    return glm::clamp((rMax - r) / kFeather, 0.0f, 1.0f);
}

} // namespace

void makeCircle(std::vector<uint8_t>& out) {
    out.resize(kSize * kSize);
    for (int y = 0; y < kSize; ++y) {
        for (int x = 0; x < kSize; ++x) {
            const float fx = (x + 0.5f) / kSize * 2.0f - 1.0f;
            const float fy = (y + 0.5f) / kSize * 2.0f - 1.0f;
            out[y * kSize + x] = static_cast<uint8_t>(circleCoverage(fx, fy) * 255.0f);
        }
    }
}

void makePolygon(std::vector<uint8_t>& out, int blades, float rotationDeg) {
    if (blades < 3) { makeCircle(out); return; }
    const float rotRad = glm::radians(rotationDeg);
    out.resize(kSize * kSize);
    for (int y = 0; y < kSize; ++y) {
        for (int x = 0; x < kSize; ++x) {
            const float fx = (x + 0.5f) / kSize * 2.0f - 1.0f;
            const float fy = (y + 0.5f) / kSize * 2.0f - 1.0f;
            out[y * kSize + x] = static_cast<uint8_t>(polygonCoverage(fx, fy, blades, rotRad) * 255.0f);
        }
    }
}

bool loadFromFile(std::vector<uint8_t>& out, const std::filesystem::path& path) {
    int w, h, ch;
    uint8_t* data = stbi_load(path.string().c_str(), &w, &h, &ch, 1);
    if (!data) {
        Log::error("Aperture", "Failed to load " + path.string());
        return false;
    }

    out.resize(kSize * kSize);
    for (int y = 0; y < kSize; ++y) {
        for (int x = 0; x < kSize; ++x) {
            const int sx = x * w / kSize;
            const int sy = y * h / kSize;
            out[y * kSize + x] = data[sy * w + sx];
        }
    }
    stbi_image_free(data);
    return true;
}

} // namespace aperture
