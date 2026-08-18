#include "gpu_packing_system.hpp"

#include <algorithm>
#include <cstring>
#include <vector>

#include "VkSmol/engine.hpp"
#include "VkSmol/frame_context.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "core/scene/gpu_structs.hpp"
#include "core/core.hpp"
#include "core/scene/scene.hpp"

namespace ecs {

namespace {
size_t sceneCapacityFromCount(size_t count) {
    size_t cap = 16;
    while (cap < count) cap <<= 1;
    return cap;
}

uint32_t resolveMaterialSlot(const ComponentStorage& materialRefs, const Entity& entity) {
    if (!materialRefs.has(entity)) return 0u;
    const Entity materialEntity = materialRefs.get(entity).get<Entity>("handle");
    const auto& materialEntities = Core::getScene().getChildren(Core::getScene().getMaterialsRoot());
    const auto found = std::find(materialEntities.begin(), materialEntities.end(), materialEntity);
    if (found == materialEntities.end()) return 0u;
    return static_cast<uint32_t>(found - materialEntities.begin());
}

uint32_t resolveMeshSlot(const ComponentStorage& meshRefs, const Entity& entity) {
    if (!meshRefs.has(entity)) return 0u;
    const Entity meshEntity = meshRefs.get(entity).get<Entity>("handle");
    const auto& assetEntities = Core::getScene().getChildren(Core::getScene().getAssetsRoot());
    const auto found = std::find(assetEntities.begin(), assetEntities.end(), meshEntity);
    if (found == assetEntities.end()) return 0u;
    return static_cast<uint32_t>(found - assetEntities.begin());
}
} // namespace

template <typename T>
inline void fillBufferWithPadding(const FrameContext& frame, SceneGpuBufferEntry& entry, std::vector<T>& data) {
    size_t required = sceneCapacityFromCount(data.size());
    if (required > entry.capacity) {
        Core::getEngine().resizeBuffer(entry.handle, required * sizeof(T));
        entry.capacity = required;
    }
    Buffer& buf = Core::getEngine().getBuffer(entry.handle, frame.currentFrame);
    data.resize(buf.getSize() / sizeof(T));
    Core::getEngine().fillBuffer(buf, data.data());
}

void spherePackingSystem(Registry& registry, const FrameContext& frame) {
    auto& spheres = registry.storage(Sphere);
    auto& transforms = registry.storage(Transform);
    auto& materialRefs = registry.storage(MaterialRef);

    std::vector<GpuSphere> gpuSpheres;

    for (const auto& entity : spheres.entities()) {
        if (!transforms.has(entity)) continue;
        const Component& sphere = spheres.get(entity);
        const Component& transform = transforms.get(entity);

        gpuSpheres.push_back(GpuSphere{
            .center = transform.get<glm::vec3>("position"),
            .radius = sphere.get<float>("radius"),
            .materialHandle = resolveMaterialSlot(materialRefs, entity),
        });
    }

    fillBufferWithPadding(frame, Core::getScene().getBuffers().sphere, gpuSpheres);
}

void planePackingSystem(Registry& registry, const FrameContext& frame) {
    auto& planes = registry.storage(Plane);
    auto& transforms = registry.storage(Transform);
    auto& materialRefs = registry.storage(MaterialRef);

    std::vector<GpuPlane> gpuPlanes;

    for (const auto& entity : planes.entities()) {
        if (!transforms.has(entity)) continue;
        const Component& transform = transforms.get(entity);
        const glm::quat rotation = glm::quat(glm::radians(transform.get<glm::vec3>("rotation")));

        gpuPlanes.push_back(GpuPlane{
            .point = transform.get<glm::vec3>("position"),
            .normal = glm::normalize(rotation * glm::vec3(0.0f, 1.0f, 0.0f)),
            .materialHandle = resolveMaterialSlot(materialRefs, entity),
        });
    }

    fillBufferWithPadding(frame, Core::getScene().getBuffers().plane, gpuPlanes);
}

void boxPackingSystem(Registry& registry, const FrameContext& frame) {
    auto& boxes = registry.storage(Box);
    auto& transforms = registry.storage(Transform);
    auto& materialRefs = registry.storage(MaterialRef);

    std::vector<GpuBox> gpuBoxes;

    for (const auto& entity : boxes.entities()) {
        if (!transforms.has(entity)) continue;
        const Component& transform = transforms.get(entity);
        const glm::mat4 local = glm::translate(glm::mat4(1.0f), transform.get<glm::vec3>("position"))
            * glm::mat4_cast(glm::quat(glm::radians(transform.get<glm::vec3>("rotation"))))
            * glm::scale(glm::mat4(1.0f), transform.get<glm::vec3>("scale"));

        gpuBoxes.push_back(GpuBox{
            .transform = local,
            .invTransform = glm::inverse(local),
            .materialHandle = resolveMaterialSlot(materialRefs, entity),
        });
    }

    fillBufferWithPadding(frame, Core::getScene().getBuffers().box, gpuBoxes);
}

void quadPackingSystem(Registry& registry, const FrameContext& frame) {
    auto& quads = registry.storage(Quad);
    auto& transforms = registry.storage(Transform);
    auto& materialRefs = registry.storage(MaterialRef);

    std::vector<GpuQuad> gpuQuads;

    for (const auto& entity : quads.entities()) {
        if (!transforms.has(entity)) continue;
        const Component& transform = transforms.get(entity);

        const glm::quat rotation = glm::quat(glm::radians(transform.get<glm::vec3>("rotation")));
        const glm::vec3 scale = transform.get<glm::vec3>("scale");
        const glm::vec3 center = transform.get<glm::vec3>("position");
        const glm::vec3 u = rotation * glm::vec3(1.0f, 0.0f, 0.0f) * scale.x;
        const glm::vec3 v = rotation * glm::vec3(0.0f, 1.0f, 0.0f) * scale.y;
        const glm::vec3 normal = rotation * glm::vec3(0.0f, 0.0f, 1.0f);
        gpuQuads.push_back(GpuQuad{
            .point = center - 0.5f * (u + v),
            .u = u,
            .v = v,
            .normal = glm::normalize(normal),
            .materialHandle = resolveMaterialSlot(materialRefs, entity),
        });
    }

    fillBufferWithPadding(frame, Core::getScene().getBuffers().quad, gpuQuads);
}

void meshPackingSystem(Registry& registry, const FrameContext& frame) {
    auto& meshRefs = registry.storage(MeshRef);
    auto& transforms = registry.storage(Transform);
    auto& materialRefs = registry.storage(MaterialRef);

    std::vector<GpuMesh> meshTemplates;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<GpuBvhNode> bvhNodes;

    const auto& assetEntities = Core::getScene().getChildren(Core::getScene().getAssetsRoot());
    for (const ecs::Entity& assetEntity : assetEntities) {
        const MeshAsset* mesh = Core::getScene().getMeshAsset(assetEntity);
        static const MeshAsset kEmptyMeshAsset;
        if (!mesh) mesh = &kEmptyMeshAsset;
        auto& meshVertices = mesh->getVertices();
        auto& meshIndices = mesh->getIndices();
        auto& meshBvhNodes = mesh->getBvhNodes();

        const bool smooth = registry.has(assetEntity, Mesh) && registry.get(assetEntity, Mesh).get<bool>("smooth");

        uint32_t vertexOffset = static_cast<uint32_t>(vertices.size());
        uint32_t indexOffset = static_cast<uint32_t>(indices.size());
        uint32_t bvhOffset = static_cast<uint32_t>(bvhNodes.size());
        glm::vec3 meshAabbMin = mesh->getAabbMin();
        glm::vec3 meshAabbMax = mesh->getAabbMax();
        meshTemplates.push_back(GpuMesh{
            .indexOffset = indexOffset,
            .triangleCount = static_cast<uint32_t>(meshIndices.size() / 3),
            .bvhOffset = bvhOffset,
            .bvhNodeCount = static_cast<uint32_t>(meshBvhNodes.size()),
            .aabbMinX = meshAabbMin.x, .aabbMinY = meshAabbMin.y, .aabbMinZ = meshAabbMin.z,
            .aabbMaxX = meshAabbMax.x, .aabbMaxY = meshAabbMax.y, .aabbMaxZ = meshAabbMax.z,
            .smoothShading = smooth ? 1u : 0u,
            .hasVertexColor = mesh->hasVertexColor() ? 1u : 0u,
        });

        vertices.insert(vertices.end(), meshVertices.begin(), meshVertices.end());
        // Offset the indices by the vertex offset
        for (size_t i = 0; i < meshIndices.size(); i++) {
            indices.push_back(meshIndices[i] + vertexOffset);
        }
        // Offset the BVH links and leaf triangle indices
        for (size_t n = 0; n < meshBvhNodes.size(); n++) {
            const GpuBvhNode& node = meshBvhNodes[n];
            GpuBvhNode packed = node;
            if (node.triangleCount > 0u) {
                packed.firstTriangle = static_cast<uint32_t>(node.firstTriangle + (indexOffset / 3));
            } else {
                packed.children[0].index = static_cast<uint32_t>(node.children[0].index + bvhOffset);
                packed.children[1].index = static_cast<uint32_t>(node.children[1].index + bvhOffset);
            }
            bvhNodes.push_back(packed);
        }
    }

    fillBufferWithPadding(frame, Core::getScene().getBuffers().vertex, vertices);
    fillBufferWithPadding(frame, Core::getScene().getBuffers().index, indices);
    fillBufferWithPadding(frame, Core::getScene().getBuffers().bvh, bvhNodes);

    std::vector<GpuMesh> meshes;

    for (const auto& entity : meshRefs.entities()) {
        if (!transforms.has(entity)) continue;
        const uint32_t meshSlot = resolveMeshSlot(meshRefs, entity);
        if (meshSlot >= static_cast<uint32_t>(meshTemplates.size())) continue;
        const GpuMesh& meshTemplate = meshTemplates[meshSlot];
        const Component& transform = transforms.get(entity);
        const glm::mat4 local = glm::translate(glm::mat4(1.0f), transform.get<glm::vec3>("position"))
            * glm::mat4_cast(glm::quat(glm::radians(transform.get<glm::vec3>("rotation"))))
            * glm::scale(glm::mat4(1.0f), transform.get<glm::vec3>("scale"));

        meshes.push_back(GpuMesh{
            .transform = local,
            .invTransform = glm::inverse(local),
            .indexOffset = meshTemplate.indexOffset,
            .triangleCount = meshTemplate.triangleCount,
            .bvhOffset = meshTemplate.bvhOffset,
            .bvhNodeCount = meshTemplate.bvhNodeCount,
            .aabbMinX = meshTemplate.aabbMinX, .aabbMinY = meshTemplate.aabbMinY, .aabbMinZ = meshTemplate.aabbMinZ,
            .aabbMaxX = meshTemplate.aabbMaxX, .aabbMaxY = meshTemplate.aabbMaxY, .aabbMaxZ = meshTemplate.aabbMaxZ,
            .materialHandle = resolveMaterialSlot(materialRefs, entity),
            .smoothShading = meshTemplate.smoothShading,
            .hasVertexColor = meshTemplate.hasVertexColor,
        });
    }

    fillBufferWithPadding(frame, Core::getScene().getBuffers().mesh, meshes);
}

void materialPackingSystem(Registry& registry, const FrameContext& frame) {
    const auto& materialEntities = Core::getScene().getChildren(Core::getScene().getMaterialsRoot());
    std::vector<GpuMaterial> gpuMaterials;
    gpuMaterials.reserve(materialEntities.size());

    for (int slot = 0; slot < static_cast<int>(materialEntities.size()); ++slot) {
        const ecs::Entity entity = materialEntities[slot];
        GpuMaterial gpu{};

        auto albedo = [&](const Component& c) {
            const glm::vec3 a = c.get<glm::vec3>("albedo");
            gpu.payload[0] = a.r; gpu.payload[1] = a.g; gpu.payload[2] = a.b;
        };

        if (registry.has(entity, ecs::Principled)) {
            const Component& c = registry.get(entity, ecs::Principled);
            gpu.type = 0;
            albedo(c);
            gpu.payload[3] = c.get<float>("roughness");
            gpu.payload[4] = c.get<float>("metalness");
            gpu.payload[5] = c.get<float>("ior");
            gpu.payload[6] = c.get<float>("transmission");
            gpu.payload[7] = c.get<float>("density");
            gpu.payload[8] = c.get<float>("anisotropic");
            gpu.payload[9] = c.get<float>("alpha");
        } else if (registry.has(entity, ecs::Emissive)) {
            const Component& c = registry.get(entity, ecs::Emissive);
            gpu.type = 1;
            albedo(c);
            gpu.payload[3] = c.get<float>("emission_strength");
        } else if (registry.has(entity, ecs::Diffuse)) {
            const Component& c = registry.get(entity, ecs::Diffuse);
            gpu.type = 2;
            albedo(c);
        } else if (registry.has(entity, ecs::Metal)) {
            const Component& c = registry.get(entity, ecs::Metal);
            gpu.type = 3;
            albedo(c);
            gpu.payload[3] = c.get<float>("roughness");
        } else if (registry.has(entity, ecs::Glossy)) {
            const Component& c = registry.get(entity, ecs::Glossy);
            gpu.type = 4;
            albedo(c);
            gpu.payload[3] = c.get<float>("roughness");
            gpu.payload[4] = c.get<float>("ior");
        } else if (registry.has(entity, ecs::Dielectric)) {
            const Component& c = registry.get(entity, ecs::Dielectric);
            gpu.type = 5;
            albedo(c);
            gpu.payload[3] = c.get<float>("roughness");
            gpu.payload[4] = c.get<float>("ior");
            gpu.payload[5] = c.get<float>("transmission");
            gpu.payload[6] = c.get<float>("density");
            gpu.payload[7] = c.get<float>("anisotropic");
        } else if (registry.has(entity, ecs::Volume)) {
            const Component& c = registry.get(entity, ecs::Volume);
            gpu.type = 6;
            albedo(c);
            gpu.payload[3] = c.get<float>("density");
            gpu.payload[4] = c.get<float>("anisotropic");
        } else if (registry.has(entity, ecs::ProgrammableMaterial)) {
            const Component& c = registry.get(entity, ecs::ProgrammableMaterial);
            gpu.type = 7;
            albedo(c);
        } else if (slot > 0 && !gpuMaterials.empty()) {
            gpu = gpuMaterials[0];
        }

        gpuMaterials.push_back(gpu);
    }

    fillBufferWithPadding(frame, Core::getScene().getBuffers().material, gpuMaterials);
}

void objectPackingSystem(Registry& registry, const FrameContext& frame) {
    const auto& transforms = registry.storage(Transform);

    std::vector<ObjectHandle> objectHandles;

    auto packType = [&](const ecs::ComponentStorage& storage, ObjectType type) {
        int idx = 0;
        for (const auto& entity : storage.entities()) {
            if (!transforms.has(entity)) continue;
            objectHandles.push_back(ObjectHandle{ .type = type, .id = idx });
            idx++;
        }
    };
    packType(registry.storage(Sphere),  ObjectType::Sphere);
    packType(registry.storage(Plane),   ObjectType::Plane);
    packType(registry.storage(Box),     ObjectType::Box);
    packType(registry.storage(Quad),    ObjectType::Quad);
    packType(registry.storage(MeshRef), ObjectType::Mesh);

    SceneGpuBufferEntry& objectEntry = Core::getScene().getBuffers().object;
    size_t objectRequired = sceneCapacityFromCount(objectHandles.size());
    if (objectRequired > objectEntry.capacity) {
        Core::getEngine().resizeBuffer(objectEntry.handle, sizeof(GpuObjectHeader) + objectRequired * sizeof(ObjectHandle));
        objectEntry.capacity = objectRequired;
    }
    uint32_t objectCount = static_cast<uint32_t>(objectHandles.size());
    objectHandles.resize(objectEntry.capacity);

    std::vector<char> objectData(sizeof(GpuObjectHeader) + sizeof(ObjectHandle) * objectEntry.capacity, 0);
    size_t offset = 0;
    std::memcpy(objectData.data() + offset, &objectCount, sizeof(objectCount));
    offset += sizeof(objectCount);
    std::memcpy(objectData.data() + offset, objectHandles.data(), objectHandles.size() * sizeof(ObjectHandle));

    Core::getEngine().fillBuffer(Core::getEngine().getBuffer(objectEntry.handle, frame.currentFrame), objectData.data());
}

void lightPackingSystem(Registry& registry, const FrameContext& frame) {
    auto& spheres = registry.storage(Sphere);
    auto& meshes = registry.storage(MeshRef);
    const auto& transforms = registry.storage(Transform);
    const auto& materialRefs = registry.storage(MaterialRef);

    auto isEmissive = [&](Entity objectEntity) -> bool {
        if (!materialRefs.has(objectEntity)) return false;
        const Entity materialEntity = materialRefs.get(objectEntity).get<Entity>("handle");
        return registry.has(materialEntity, ecs::Emissive);
    };

    std::vector<GpuLight> lights;
    int32_t objectId = 0;
    float totalArea = 0.0f;

    // Spheres
    for (const auto& entity : registry.storage(Sphere).entities()) {
        if (!transforms.has(entity)) continue;
        objectId++;
        if (!isEmissive(entity)) continue;

        const float area = 4.0f * glm::pi<float>() * std::pow(spheres.get(entity).get<float>("radius"), 2.0f);
        totalArea += area;
        lights.push_back(GpuLight{
            .objectId = objectId-1,
            .area = area,
            .pdfA = 1.0f / area,
        });
    }
    // Planes can't be used for importance sampling (infinite area)
    for (const auto& entity : registry.storage(Plane).entities()) {
        if (!transforms.has(entity)) continue;
        objectId++;
    }
    // Boxes
    for (const auto& entity : registry.storage(Box).entities()) {
        if (!transforms.has(entity)) continue;
        objectId++;
        if (!isEmissive(entity)) continue;

        const Component& boxTransform = transforms.get(entity);
        const glm::mat4 local = glm::translate(glm::mat4(1.0f), boxTransform.get<glm::vec3>("position"))
            * glm::mat4_cast(glm::quat(glm::radians(boxTransform.get<glm::vec3>("rotation"))))
            * glm::scale(glm::mat4(1.0f), boxTransform.get<glm::vec3>("scale"));
        const glm::vec3 axisX = glm::vec3(local[0]);
        const glm::vec3 axisY = glm::vec3(local[1]);
        const glm::vec3 axisZ = glm::vec3(local[2]);
        const float hx = glm::length(axisX);
        const float hy = glm::length(axisY);
        const float hz = glm::length(axisZ);
        const float area = 8.0f * (hx * hy + hx * hz + hy * hz);
        totalArea += area;
        lights.push_back(GpuLight{
            .objectId = objectId-1,
            .area = area,
            .pdfA = 1.0f / area,
        });
    }
    // Quads
    for (const auto& entity : registry.storage(Quad).entities()) {
        if (!transforms.has(entity)) continue;
        objectId++;
        if (!isEmissive(entity)) continue;

        const Component& quadTransform = transforms.get(entity);
        const glm::mat4 local = glm::translate(glm::mat4(1.0f), quadTransform.get<glm::vec3>("position"))
            * glm::mat4_cast(glm::quat(glm::radians(quadTransform.get<glm::vec3>("rotation"))))
            * glm::scale(glm::mat4(1.0f), quadTransform.get<glm::vec3>("scale"));
        const glm::vec3 u = glm::vec3(local[0]);
        const glm::vec3 v = glm::vec3(local[1]);
        const float area = glm::length(glm::cross(u, v));
        totalArea += area;
        lights.push_back(GpuLight{
            .objectId = objectId-1,
            .area = area,
            .pdfA = 1.0f / area,
        });
    }
    // Meshes
    for (const auto& entity : registry.storage(MeshRef).entities()) {
        if (!transforms.has(entity)) continue;
        objectId++;
        if (!isEmissive(entity)) continue;

        const Component& meshTransform = transforms.get(entity);
        const glm::mat4 mLocal = glm::translate(glm::mat4(1.0f), meshTransform.get<glm::vec3>("position"))
            * glm::mat4_cast(glm::quat(glm::radians(meshTransform.get<glm::vec3>("rotation"))))
            * glm::scale(glm::mat4(1.0f), meshTransform.get<glm::vec3>("scale"));
        const Entity meshAssetEntity = meshes.get(entity).get<Entity>("handle");
        const MeshAsset* meshAsset = Core::getScene().getMeshAsset(meshAssetEntity);
        if (!meshAsset) continue;
        const float area = meshAsset->computeArea(mLocal);
        totalArea += area;
        lights.push_back(GpuLight{
            .objectId = objectId-1,
            .area = area,
            .pdfA = 1.0f / area,
        });
    }

    SceneGpuBufferEntry& lightEntry = Core::getScene().getBuffers().light;
    size_t lightRequired = sceneCapacityFromCount(lights.size());
    if (lightRequired > lightEntry.capacity) {
        Core::getEngine().resizeBuffer(lightEntry.handle, sizeof(GpuLightHeader) + lightRequired * sizeof(GpuLight));
        lightEntry.capacity = lightRequired;
    }
    lights.resize(lightEntry.capacity);

    std::vector<char> lightData(sizeof(GpuLightHeader) + sizeof(GpuLight) * lightEntry.capacity, 0);
    size_t offset = 0;
    std::memcpy(lightData.data() + offset, &totalArea, sizeof(totalArea));
    offset += sizeof(totalArea);
    std::memcpy(lightData.data() + offset, lights.data(), lights.size() * sizeof(GpuLight));

    Core::getEngine().fillBuffer(Core::getEngine().getBuffer(lightEntry.handle, frame.currentFrame), lightData.data());
}

} // namespace ecs
