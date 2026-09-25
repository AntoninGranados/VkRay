#include "gpu_packing_system.hpp"

#include <algorithm>
#include <cstring>
#include <utility>
#include <vector>

#include "VkSmol/engine.hpp"
#include "VkSmol/frame_context.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "core/ecs/components/environment.hpp"
#include "core/ecs/components/geometry.hpp"
#include "core/ecs/systems/motion_sampling.hpp"
#include "core/render/camera_lens_table.hpp"
#include "core/render/material_table.hpp"
#include "core/render/sky_table.hpp"
#include "core/scene/asset/mesh.hpp"
#include "core/scene/gpu_structs.hpp"
#include "core/core.hpp"
#include "core/scene/scene.hpp"

#include "utils/log.hpp"
#include "utils/math_utils.hpp"

namespace ecs {

namespace {
uint32_t resolveMaterialSlot(Registry& registry, const ComponentStorage& materialRefs, const Entity& entity) {
    if (!materialRefs.has(entity)) return 0u;
    const Entity materialEntity = materialRefs.get(entity).get<Entity>("handle");
    const auto& materialEntities = registry.getChildren(registry.ctx().get<SceneRoots>().materialsRoot);
    const auto found = std::find(materialEntities.begin(), materialEntities.end(), materialEntity);
    if (found == materialEntities.end()) return 0u;
    return static_cast<uint32_t>(found - materialEntities.begin());
}

uint32_t resolveMeshSlot(Registry& registry, const ComponentStorage& meshRefs, const Entity& entity) {
    if (!meshRefs.has(entity)) return 0u;
    const Entity meshEntity = meshRefs.get(entity).get<Entity>("handle");
    const auto& assetEntities = registry.getChildren(registry.ctx().get<SceneRoots>().assetsRoot);
    const auto found = std::find(assetEntities.begin(), assetEntities.end(), meshEntity);
    if (found == assetEntities.end()) return 0u;
    return static_cast<uint32_t>(found - assetEntities.begin());
}

const MeshAsset* getMeshAsset(Registry& registry, Entity e) {
    return registry.has(e, Mesh) ? &registry.get(e, Mesh).payload<MeshAsset>("geometry") : nullptr;
}

Entity resolveEnvironmentEntity(Registry& registry) {
    for (const Entity& e : registry.getChildren(registry.ctx().get<SceneRoots>().sceneRoot))
        if (registry.has(e, Environment)) return e;
    return {};
}

glm::mat4 composeTransform(const Component& transform) {
    return glm::translate(glm::mat4(1.0f), transform.get<glm::vec3>("position"))
        * glm::mat4_cast(glm::quat(glm::radians(transform.get<glm::vec3>("rotation"))))
        * glm::scale(glm::mat4(1.0f), transform.get<glm::vec3>("scale"));
}

} // namespace

const std::vector<const ComponentType*>& objectTypeOrder() {
    static const std::vector<const ComponentType*> order = [] {
        std::vector<const ComponentType*> result;
        for (const ComponentType& type : ComponentType::all())
            if (type.getGroup() == "object") result.push_back(&type);
        return result;
    }();
    return order;
}

template <typename T>
inline void fillBufferWithPadding(const FrameContext& frame, SceneGpuBufferEntry& entry, std::vector<T>& data) {
    size_t required = nextPowerOfTwo(data.size());
    if (required > entry.capacity) {
        Core::getEngine().resizeBuffer(entry.handle, required * sizeof(T));
        entry.capacity = required;
    }
    Buffer& buf = Core::getEngine().getBuffer(entry.handle, frame.currentFrame);
    data.resize(buf.getSize() / sizeof(T));
    Core::getEngine().fillBuffer(buf, data.data());
}

template <typename Header, typename T>
inline void fillBufferWithHeader(const FrameContext& frame, SceneGpuBufferEntry& entry, const Header& header, std::vector<T>& data) {
    size_t required = nextPowerOfTwo(data.size());
    if (required > entry.capacity) {
        Core::getEngine().resizeBuffer(entry.handle, sizeof(Header) + required * sizeof(T));
        entry.capacity = required;
    }
    data.resize(entry.capacity);

    std::vector<char> buffer(sizeof(Header) + sizeof(T) * entry.capacity, 0);
    std::memcpy(buffer.data(), &header, sizeof(Header));
    std::memcpy(buffer.data() + sizeof(Header), data.data(), data.size() * sizeof(T));

    Core::getEngine().fillBuffer(Core::getEngine().getBuffer(entry.handle, frame.currentFrame), buffer.data());
}

void meshPackingSystem(Registry& registry) {
    const FrameContext& frame = registry.ctx().get<FrameContext>();
    auto& meshRefs = registry.storage(MeshRef);
    auto& transforms = registry.storage(Transform);

    std::vector<GpuMesh> meshTemplates;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<GpuBvhNode> bvhNodes;

    const auto& assetEntities = registry.getChildren(registry.ctx().get<SceneRoots>().assetsRoot);
    for (const ecs::Entity& assetEntity : assetEntities) {
        const MeshAsset* mesh = getMeshAsset(registry, assetEntity);
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

    fillBufferWithPadding(frame, registry.ctx().get<SceneGpuBuffers>().vertex, vertices);
    fillBufferWithPadding(frame, registry.ctx().get<SceneGpuBuffers>().index, indices);
    fillBufferWithPadding(frame, registry.ctx().get<SceneGpuBuffers>().bvh, bvhNodes);

    std::vector<GpuMesh> meshes;

    for (const auto& entity : meshRefs.entities()) {
        if (!transforms.has(entity)) continue;
        const uint32_t meshSlot = resolveMeshSlot(registry, meshRefs, entity);
        if (meshSlot >= static_cast<uint32_t>(meshTemplates.size())) continue;
        meshes.push_back(meshTemplates[meshSlot]);
    }

    fillBufferWithPadding(frame, registry.ctx().get<SceneGpuBuffers>().mesh, meshes);
}

void materialPackingSystem(Registry& registry) {
    const FrameContext& frame = registry.ctx().get<FrameContext>();
    const auto& materialEntities = registry.getChildren(registry.ctx().get<SceneRoots>().materialsRoot);

    std::vector<GpuMaterial> gpuMaterials;
    std::vector<float> pluginParams;
    gpuMaterials.reserve(materialEntities.size());

    for (const ecs::Entity& entity : materialEntities) {
        GpuMaterial gpu{};
        MaterialTable::pack(registry, entity, gpu, pluginParams);
        gpuMaterials.push_back(gpu);
    }

    // Slack so unpackMaterial's fixed-size read past the last material's base never goes out of bounds.
    pluginParams.resize(pluginParams.size() + kMaterialPayloadSize, 0.0f);

    LensPluginInfo& lensInfo = registry.ctx().get<LensPluginInfo>();
    lensInfo.paramsBase = static_cast<int32_t>(pluginParams.size());
    lensInfo.slot = CameraLensTable::pack(registry, *registry.ctx().get<Entity*>(), pluginParams);

    SkyPluginInfo& skyInfo = registry.ctx().get<SkyPluginInfo>();
    const size_t skyBase = pluginParams.size();
    SkyTable::pack(registry, resolveEnvironmentEntity(registry), pluginParams);
    skyInfo.paramsBase = pluginParams.size() > skyBase ? static_cast<int32_t>(skyBase) : -1;

    fillBufferWithPadding(frame, registry.ctx().get<SceneGpuBuffers>().material, gpuMaterials);
    fillBufferWithPadding(frame, registry.ctx().get<SceneGpuBuffers>().pluginParams, pluginParams);
}

void objectPackingSystem(Registry& registry) {
    const FrameContext& frame = registry.ctx().get<FrameContext>();
    auto& transforms = registry.storage(Transform);
    const auto& materialRefs = registry.storage(MaterialRef);

    std::vector<GpuObject> gpuObjects;
    std::vector<GpuMotionSample> motion;
    std::vector<GpuMotionSample> liveMotion;

    const std::vector<const ComponentType*>& order = objectTypeOrder();
    for (size_t i = 0; i < order.size(); i++) {
        const ObjectType objectType = static_cast<ObjectType>(i + 1);
        uint32_t idx = 0;
        for (const auto& entity : registry.storage(*order[i]).entities()) {
            if (!transforms.has(entity)) continue;
            const uint32_t motionOffset = bakeMotionSamples(registry, entity, transforms.get(entity), motion);
            bakeLiveTransform(transforms.get(entity), liveMotion);
            gpuObjects.push_back(GpuObject{
                .type = objectType,
                .id = idx,
                .materialSlot = resolveMaterialSlot(registry, materialRefs, entity),
                .motionOffset = motionOffset,
            });
            idx++;
        }
    }

    const Entity cameraEntity = *registry.ctx().get<Entity*>();
    CameraMotionInfo& cameraMotion = registry.ctx().get<CameraMotionInfo>();
    if (transforms.has(cameraEntity)) {
        const uint32_t motionOffset = bakeMotionSamples(registry, cameraEntity, transforms.get(cameraEntity), motion);
        bakeLiveTransform(transforms.get(cameraEntity), liveMotion);
        cameraMotion = CameraMotionInfo{ motionOffset };
    } else {
        cameraMotion = CameraMotionInfo{};
    }

    const GpuObjectHeader header{ .objectCount = static_cast<uint32_t>(gpuObjects.size()) };
    fillBufferWithHeader(frame, registry.ctx().get<SceneGpuBuffers>().object, header, gpuObjects);
    fillBufferWithPadding(frame, registry.ctx().get<SceneGpuBuffers>().motion, motion);
    fillBufferWithPadding(frame, registry.ctx().get<SceneGpuBuffers>().liveMotion, liveMotion);
}

void lightPackingSystem(Registry& registry) {
    const FrameContext& frame = registry.ctx().get<FrameContext>();
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

    for (const ComponentType* type : objectTypeOrder()) {
        for (const auto& entity : registry.storage(*type).entities()) {
            if (!transforms.has(entity)) continue;
            objectId++;

            // Planes can't be used for importance sampling (infinite area)
            if (type == &Plane) continue;
            if (!isEmissive(entity)) continue;

            float area;
            if (*type == Sphere) {
                const Component& sphereTransform = transforms.get(entity);
                const glm::mat4 local = composeTransform(sphereTransform);
                const glm::vec3 axisX = glm::vec3(local[0]);
                const glm::vec3 axisY = glm::vec3(local[1]);
                const glm::vec3 axisZ = glm::vec3(local[2]);
                const float radius = (glm::length(axisX) + glm::length(axisY) + glm::length(axisZ)) / 3.0f;
                area = 4.0f * glm::pi<float>() * radius * radius;
            } else if (*type == Box) {
                const Component& boxTransform = transforms.get(entity);
                const glm::mat4 local = composeTransform(boxTransform);
                const glm::vec3 axisX = glm::vec3(local[0]);
                const glm::vec3 axisY = glm::vec3(local[1]);
                const glm::vec3 axisZ = glm::vec3(local[2]);
                const float hx = glm::length(axisX);
                const float hy = glm::length(axisY);
                const float hz = glm::length(axisZ);
                area = 8.0f * (hx * hy + hx * hz + hy * hz);
            } else if (*type == Quad) {
                const Component& quadTransform = transforms.get(entity);
                const glm::mat4 local = composeTransform(quadTransform);
                const glm::vec3 u = glm::vec3(local[0]);
                const glm::vec3 v = glm::vec3(local[1]);
                area = glm::length(glm::cross(u, v));
            } else if (*type == MeshRef) {
                const Component& meshTransform = transforms.get(entity);
                const glm::mat4 mLocal = composeTransform(meshTransform);
                const Entity meshAssetEntity = meshes.get(entity).get<Entity>("handle");
                const MeshAsset* meshAsset = getMeshAsset(registry, meshAssetEntity);
                if (!meshAsset) continue;
                area = meshAsset->computeArea(mLocal);
            } else {
                std::unreachable();
            }

            totalArea += area;
            lights.push_back(GpuLight{
                .objectId = objectId - 1,
                .area = area,
                .pdfA = 1.0f / area,
            });
        }
    }

    const GpuLightHeader header{ .totalArea = totalArea };
    fillBufferWithHeader(frame, registry.ctx().get<SceneGpuBuffers>().light, header, lights);
}

} // namespace ecs
