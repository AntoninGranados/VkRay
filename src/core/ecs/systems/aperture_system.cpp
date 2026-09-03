#include "aperture_system.hpp"

#include <filesystem>

#include "core/camera/aperture.hpp"
#include "core/core.hpp"
#include "core/ecs/components.hpp"

namespace ecs {

void apertureSystem(Registry& registry) {
    ApertureState& state = registry.ctx().get<ApertureState>();

    VkSmol& engine = Core::getEngine();
    auto upload = [&](std::vector<uint8_t> data) {
        engine.fillImage(engine.getImage(Core::getCoreRenderer().getLensImageHandle()), data.data(), data.size());
    };

    auto& geometrics = registry.storage(GeometricAperture);
    for (const auto& e : geometrics.entities()) {
        const Component& c = geometrics.get(e);
        int   blades   = c.get<int>("blades");
        float rotation = c.get<float>("rotation");
        if (blades != state.lastBlades || rotation != state.lastRotation) {
            std::vector<uint8_t> data;
            aperture::makePolygon(data, blades, rotation);
            upload(std::move(data));
            state.lastBlades      = blades;
            state.lastRotation    = rotation;
            state.defaultUploaded = false;
        }
        return;
    }

    auto& images = registry.storage(ImageAperture);
    for (const auto& e : images.entities()) {
        const Component& c = images.get(e);
        auto path = c.get<std::filesystem::path>("path");
        if (path != state.lastPath && !path.empty()) {
            std::vector<uint8_t> data;
            if (aperture::loadFromFile(data, path)) {
                upload(std::move(data));
                state.lastPath        = path;
                state.defaultUploaded = false;
            }
        }
        return;
    }

    if (!state.defaultUploaded && !registry.storage(ThinLens).entities().empty()) {
        std::vector<uint8_t> data;
        aperture::makeCircle(data);
        upload(std::move(data));
        state.defaultUploaded = true;
        state.lastBlades      = -1;
        state.lastRotation    = -1.0f;
        state.lastPath.clear();
    }
}

} // namespace ecs
