#include "gpu_packing_system.hpp"

#include <algorithm>
#include <cstring>
#include <vector>

#include "VkSmol/engine.hpp"
#include "VkSmol/frame_context.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "core/render/material_table.hpp"
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

void spherePackingSystem(Registry& registry) {
    const FrameContext& frame = registry.ctx().get<FrameContext>();
    auto& spheres = registry.storage(Sphere);
    auto& transforms = registry.storage(Transform);

    std::vector<GpuSphere> gpuSpheres;

    for (const auto& entity : spheres.entities()) {
        if (!transforms.has(entity)) continue;
        const Component& sphere = spheres.get(entity);
        const Component& transform = transforms.get(entity);

        gpuSpheres.push_back(GpuSphere{
            .center = transform.get<glm::vec3>("position"),
            .radius = sphere.get<float>("radius"),
        });
    }

    fillBufferWithPadding(frame, registry.ctx().get<SceneGpuBuffers>().sphere, gpuSpheres);
}

void planePackingSystem(Registry& registry) {
    const FrameContext& frame = registry.ctx().get<FrameContext>();
    auto& planes = registry.storage(Plane);
    auto& transforms = registry.storage(Transform);

    std::vector<GpuPlane> gpuPlanes;

    for (const auto& entity : planes.entities()) {
        if (!transforms.has(entity)) continue;
        const Component& transform = transforms.get(entity);
        const glm::quat rotation = glm::quat(glm::radians(transform.get<glm::vec3>("rotation")));

        gpuPlanes.push_back(GpuPlane{
            .point = transform.get<glm::vec3>("position"),
            .normal = glm::normalize(rotation * glm::vec3(0.0f, 1.0f, 0.0f)),
        });
    }

    fillBufferWithPadding(frame, registry.ctx().get<SceneGpuBuffers>().plane, gpuPlanes);
}

void boxPackingSystem(Registry& registry) {
    const FrameContext& frame = registry.ctx().get<FrameContext>();
    auto& boxes = registry.storage(Box);
    auto& transforms = registry.storage(Transform);

    std::vector<GpuBox> gpuBoxes;

    for (const auto& entity : boxes.entities()) {
        if (!transforms.has(entity)) continue;
        const Component& transform = transforms.get(entity);
        const glm::mat4 local = composeTransform(transform);

        gpuBoxes.push_back(GpuBox{
            .transform = local,
            .invTransform = glm::inverse(local),
        });
    }

    fillBufferWithPadding(frame, registry.ctx().get<SceneGpuBuffers>().box, gpuBoxes);
}

void quadPackingSystem(Registry& registry) {
    const FrameContext& frame = registry.ctx().get<FrameContext>();
    auto& quads = registry.storage(Quad);
    auto& transforms = registry.storage(Transform);

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
        });
    }

    fillBufferWithPadding(frame, registry.ctx().get<SceneGpuBuffers>().quad, gpuQuads);
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
        const GpuMesh& meshTemplate = meshTemplates[meshSlot];
        const Component& transform = transforms.get(entity);
        const glm::mat4 local = composeTransform(transform);

        meshes.push_back(GpuMesh{
            .transform = local,
            .invTransform = glm::inverse(local),
            .indexOffset = meshTemplate.indexOffset,
            .triangleCount = meshTemplate.triangleCount,
            .bvhOffset = meshTemplate.bvhOffset,
            .bvhNodeCount = meshTemplate.bvhNodeCount,
            .aabbMinX = meshTemplate.aabbMinX, .aabbMinY = meshTemplate.aabbMinY, .aabbMinZ = meshTemplate.aabbMinZ,
            .aabbMaxX = meshTemplate.aabbMaxX, .aabbMaxY = meshTemplate.aabbMaxY, .aabbMaxZ = meshTemplate.aabbMaxZ,
            .smoothShading = meshTemplate.smoothShading,
            .hasVertexColor = meshTemplate.hasVertexColor,
        });
    }

    fillBufferWithPadding(frame, registry.ctx().get<SceneGpuBuffers>().mesh, meshes);
}

void materialPackingSystem(Registry& registry) {
    const FrameContext& frame = registry.ctx().get<FrameContext>();
    const auto& materialEntities = registry.getChildren(registry.ctx().get<SceneRoots>().materialsRoot);

    std::vector<GpuMaterial> gpuMaterials;
    std::vector<float> materialParams;
    gpuMaterials.reserve(materialEntities.size());

    for (int slot = 0; slot < static_cast<int>(materialEntities.size()); ++slot) {
        const ecs::Entity entity = materialEntities[slot];
        GpuMaterial gpu{};

        if (!MaterialTable::pack(registry, entity, gpu, materialParams) && slot > 0 && !gpuMaterials.empty())
            gpu = gpuMaterials[0];

        gpuMaterials.push_back(gpu);
    }

    // Slack so unpackMaterial's fixed-size read past the last material's base never goes out of bounds.
    materialParams.resize(materialParams.size() + kMaterialPayloadSize, 0.0f);

    fillBufferWithPadding(frame, registry.ctx().get<SceneGpuBuffers>().material, gpuMaterials);
    fillBufferWithPadding(frame, registry.ctx().get<SceneGpuBuffers>().materialParams, materialParams);
}

void objectPackingSystem(Registry& registry) {
    const FrameContext& frame = registry.ctx().get<FrameContext>();
    const auto& transforms = registry.storage(Transform);
    const auto& materialRefs = registry.storage(MaterialRef);

    std::vector<GpuObject> gpuObjects;

    const std::vector<const ComponentType*>& order = objectTypeOrder();
    for (size_t i = 0; i < order.size(); i++) {
        const ObjectType objectType = static_cast<ObjectType>(i + 1);
        uint32_t idx = 0;
        for (const auto& entity : registry.storage(*order[i]).entities()) {
            if (!transforms.has(entity)) continue;
            gpuObjects.push_back(GpuObject{
                .type = objectType,
                .id = idx,
                .materialSlot = resolveMaterialSlot(registry, materialRefs, entity)
            });
            idx++;
        }
    }

    const GpuObjectHeader header{ .objectCount = static_cast<uint32_t>(gpuObjects.size()) };
    fillBufferWithHeader(frame, registry.ctx().get<SceneGpuBuffers>().object, header, gpuObjects);
}

void lightPackingSystem(Registry& registry) {
    const FrameContext& frame = registry.ctx().get<FrameContext>();
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
        const glm::mat4 local = composeTransform(boxTransform);
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
        const glm::mat4 local = composeTransform(quadTransform);
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
        const glm::mat4 mLocal = composeTransform(meshTransform);
        const Entity meshAssetEntity = meshes.get(entity).get<Entity>("handle");
        const MeshAsset* meshAsset = getMeshAsset(registry, meshAssetEntity);
        if (!meshAsset) continue;
        const float area = meshAsset->computeArea(mLocal);
        totalArea += area;
        lights.push_back(GpuLight{
            .objectId = objectId-1,
            .area = area,
            .pdfA = 1.0f / area,
        });
    }

    const GpuLightHeader header{ .totalArea = totalArea };
    fillBufferWithHeader(frame, registry.ctx().get<SceneGpuBuffers>().light, header, lights);
}

} // namespace ecs
