#include "transform_system.hpp"

#include <vector>

#include <glm/gtc/matrix_transform.hpp>

#include "../components.hpp"

#include "../../scene/scene.hpp"
#include "../../scene/object/object.hpp"

namespace ecs {

void spherePackingSystem(Registry& registry, AppContext& ctx) {
    auto& spheres = registry.storage<ecs::Sphere>();
    auto& transforms = registry.storage<ecs::Transform>();
    auto& materialRefs = registry.storage<ecs::MaterialRef>();
    auto& packingMaps = ctx.scene->getPackingMaps();

    std::vector<GpuSphere> gpuSpheres;
    packingMaps.sphereId.clear();
    size_t sphereId = 0;

    for (const auto& e : spheres.entities()) {
        if (!transforms.has(e)) continue;
        Sphere& s = spheres.get(e);
        Transform& t = transforms.get(e);

        MaterialHandle handle = materialRefs.has(e) ? materialRefs.get(e).handle : 0;
        
        gpuSpheres.push_back(GpuSphere{
            .center = t.position,
            .radius = s.radius,
            .materialHandle = handle,
        });
                
        // const float area = 4.0f * glm::pi<float>() * sphere.radius * sphere.radius;
        // addLight(materials[spheres[sphereId].materialHandle], area, entityCount, lightCount, lights, totalLightArea);
        packingMaps.sphereId[e] = sphereId++;
    }

    auto& sphereBuffers = ctx.scene->getSphereBuffers();
    sphereBuffers.setElementCount(*ctx.engine, sphereId);
    sphereBuffers.fill(*ctx.engine, gpuSpheres.data());
}

void planePackingSystem(Registry& registry, AppContext& ctx) {
    auto& planes = registry.storage<ecs::Plane>();
    auto& transforms = registry.storage<ecs::Transform>();
    auto& materialRefs = registry.storage<ecs::MaterialRef>();
    auto& packingMaps = ctx.scene->getPackingMaps();

    std::vector<GpuPlane> gpuPlanes;
    packingMaps.planeId.clear();
    size_t planeId = 0;

    for (const auto& e : planes.entities()) {
        if (!transforms.has(e)) continue;
        // Plane& p = planes.get(e);
        Transform& t = transforms.get(e);

        MaterialHandle handle = materialRefs.has(e) ? materialRefs.get(e).handle : 0;
        
        gpuPlanes.push_back(GpuPlane{
            .point = t.position,
            .normal = glm::normalize(t.rotation * glm::vec3(0.0f, 1.0f, 0.0f)),
            .materialHandle = handle,
        });
        
        packingMaps.planeId[e] = planeId++;
    }

    auto& planeBuffers = ctx.scene->getPlaneBuffers();
    planeBuffers.setElementCount(*ctx.engine, planeId);
    planeBuffers.fill(*ctx.engine, gpuPlanes.data());
}

void boxPackingSystem(Registry& registry, AppContext& ctx) {
    auto& boxes = registry.storage<ecs::Box>();
    auto& transforms = registry.storage<ecs::Transform>();
    auto& materialRefs = registry.storage<ecs::MaterialRef>();
    auto& packingMaps = ctx.scene->getPackingMaps();

    std::vector<GpuBox> gpuBoxes;
    packingMaps.boxId.clear();
    size_t boxId = 0;

    for (const auto& e : boxes.entities()) {
        if (!transforms.has(e)) continue;
        // Box& b = boxes.get(e);
        Transform& t = transforms.get(e);

        MaterialHandle handle = materialRefs.has(e) ? materialRefs.get(e).handle : 0;
        
        gpuBoxes.push_back(GpuBox{
            .transform = t.local,
            .invTransform = glm::inverse(t.local),
            .materialHandle = handle,
        });

        // const glm::vec3 axisX = glm::vec3(transform.local[0]);
        // const glm::vec3 axisY = glm::vec3(transform.local[1]);
        // const glm::vec3 axisZ = glm::vec3(transform.local[2]);
        // const float hx = glm::length(axisX);
        // const float hy = glm::length(axisY);
        // const float hz = glm::length(axisZ);
        // const float area = 8.0f * (hx * hy + hx * hz + hy * hz);
        // addLight(materials[handle], area, entityCount, lightCount, lights, totalLightArea);
        packingMaps.boxId[e] = boxId++;
    }

    auto& boxBuffers = ctx.scene->getBoxBuffers();
    boxBuffers.setElementCount(*ctx.engine, boxId);
    boxBuffers.fill(*ctx.engine, gpuBoxes.data());
}

void meshPackingSystem(Registry& registry, AppContext& ctx) {
    // TODO: pack vertices, indices, BVHs (ie. mesh structure)
    // TODO: pach mesh (ie. mesh objects/mesh refs)
}

} // namespace ecs
