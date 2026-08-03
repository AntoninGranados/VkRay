#include "aperture_system.hpp"

#include <filesystem>

#include "core/camera/aperture.hpp"
#include "core/core.hpp"
#include "core/ecs/components.hpp"

namespace ecs {

void apertureSystem(Registry& registry) {
    static int lastBlades = -1;
    static float lastRotation = -1.0f;
    static std::filesystem::path lastPath;
    static bool defaultUploaded = false;

    VkSmol& engine = Core::getEngine();
    auto upload = [&](std::vector<uint8_t> data) {
        engine.fillImage(engine.getImage(Core::getCoreRenderer().getLensImageHandle()), data.data(), data.size());
    };

    auto& geometrics = registry.storage(GeometricAperture);
    for (const auto& e : geometrics.entities()) {
        const Component& c = geometrics.get(e);
        int   blades   = c.get<int>("blades");
        float rotation = c.get<float>("rotation");
        if (blades != lastBlades || rotation != lastRotation) {
            std::vector<uint8_t> data;
            aperture::makePolygon(data, blades, rotation);
            upload(std::move(data));
            lastBlades      = blades;
            lastRotation    = rotation;
            defaultUploaded = false;
        }
        return;
    }

    auto& images = registry.storage(ImageAperture);
    for (const auto& e : images.entities()) {
        const Component& c = images.get(e);
        auto path = c.get<std::filesystem::path>("path");
        if (path != lastPath && !path.empty()) {
            std::vector<uint8_t> data;
            if (aperture::loadFromFile(data, path)) {
                upload(std::move(data));
                lastPath        = path;
                defaultUploaded = false;
            }
        }
        return;
    }

    if (!defaultUploaded && !registry.storage(ThinLensCamera).entities().empty()) {
        std::vector<uint8_t> data;
        aperture::makeCircle(data);
        upload(std::move(data));
        defaultUploaded = true;
        lastBlades      = -1;
        lastRotation    = -1.0f;
        lastPath.clear();
    }
}

} // namespace ecs
