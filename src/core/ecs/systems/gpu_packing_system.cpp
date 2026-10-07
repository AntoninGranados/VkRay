#include "gpu_packing_system.hpp"

#include <algorithm>
#include <unordered_map>
#include <utility>
#include <vector>

#include "VkSmol/engine.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/intersect.hpp>

#include "core/core.hpp"
#include "core/ecs/components/environment.hpp"
#include "core/ecs/components/geometry.hpp"
#include "core/ecs/systems/motion_sampling.hpp"
#include "core/render/camera_lens_table.hpp"
#include "core/render/compositing_table.hpp"
#include "core/render/material_table.hpp"
#include "core/render/sky_table.hpp"
#include "core/scene/asset/mesh.hpp"
#include "core/scene/gpu_structs.hpp"
#include "core/scene/scene.hpp"

#include "utils/color_utils.hpp"
#include "utils/log.hpp"

namespace ecs {

namespace {
using EntitySlots = std::unordered_map<Entity, uint32_t>;

EntitySlots buildEntitySlots(const std::vector<Entity>& entities) {
    EntitySlots slots;
    slots.reserve(entities.size());
    for (size_t i = 0; i < entities.size(); i++) slots.emplace(entities[i], static_cast<uint32_t>(i));
    return slots;
}

uint32_t resolveMaterialSlot(const EntitySlots& materialSlots, const ComponentStorage& materialRefs,
                             const Entity& entity) {
    if (!materialRefs.has(entity)) return 0u;
    const auto found = materialSlots.find(materialRefs.get(entity).get<Entity>("handle"));
    return found == materialSlots.end() ? 0u : found->second;
}

uint32_t resolveMeshSlot(const EntitySlots& meshSlots, const ComponentStorage& meshRefs, const Entity& entity) {
    if (!meshRefs.has(entity)) return 0u;
    const auto found = meshSlots.find(meshRefs.get(entity).get<Entity>("handle"));
    return found == meshSlots.end() ? 0u : found->second;
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
    return glm::translate(glm::mat4(1.0f), transform.get<glm::vec3>("position")) *
           glm::mat4_cast(glm::quat(glm::radians(transform.get<glm::vec3>("rotation")))) *
           glm::scale(glm::mat4(1.0f), transform.get<glm::vec3>("scale"));
}

bool isMediumCandidate(Registry& registry, const ComponentStorage& materialRefs, const Entity& entity) {
    if (!materialRefs.has(entity)) return false;
    const Entity material = materialRefs.get(entity).get<Entity>("handle");
    return registry.has(material, Dielectric) || registry.has(material, Volume) || registry.has(material, Principled) ||
           registry.has(material, MaterialPlugin);
}

bool meshContains(const MeshAsset& mesh, const glm::vec3& point) {
    if (glm::any(glm::lessThan(point, mesh.getAabbMin())) || glm::any(glm::greaterThan(point, mesh.getAabbMax())))
        return false;

    const glm::vec3 direction = glm::normalize(glm::vec3(1.0f, 0.37f, 0.21f));
    const std::vector<Vertex>& vertices = mesh.getVertices();
    const std::vector<uint32_t>& indices = mesh.getIndices();
    int crossings = 0;
    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        glm::vec2 barycentric;
        float distance;
        if (glm::intersectRayTriangle(point, direction, vertices[indices[i]].position,
                                      vertices[indices[i + 1]].position, vertices[indices[i + 2]].position, barycentric,
                                      distance) &&
            distance > 0.0f)
            crossings++;
    }
    return crossings % 2 == 1;
}

float containingVolume(Registry& registry, const ComponentType& type, const Entity& entity, const glm::mat4& model,
                       const glm::vec3& point) {
    const glm::vec3 local = glm::vec3(glm::inverse(model) * glm::vec4(point, 1.0f));
    const float scale = std::abs(glm::determinant(model));

    if (type == Sphere) return glm::length(local) < 1.0f ? scale * 4.0f / 3.0f * glm::pi<float>() : 0.0f;
    if (type == Box) return glm::all(glm::lessThan(glm::abs(local), glm::vec3(1.0f))) ? scale * 8.0f : 0.0f;
    if (type != MeshRef) return 0.0f;

    const MeshAsset* mesh = getMeshAsset(registry, registry.get(entity, MeshRef).get<Entity>("handle"));
    if (!mesh || !meshContains(*mesh, local)) return 0.0f;
    const glm::vec3 extent = mesh->getAabbMax() - mesh->getAabbMin();
    return scale * extent.x * extent.y * extent.z;
}

float emittedLuminance(Registry& registry, const Entity& materialEntity) {
    const Component& emissive = registry.get(materialEntity, Emissive);
    return luminance(emissive.get<glm::vec3>("albedo") * emissive.get<float>("emission_strength"));
}

bool isSampledLight(Registry& registry, const ComponentStorage& materialRefs, const ComponentType& type,
                    const Entity& entity) {
    // Planes can't be used for importance sampling (infinite area)
    if (type == Plane || !materialRefs.has(entity)) return false;

    const Entity materialEntity = materialRefs.get(entity).get<Entity>("handle");
    if (!registry.has(materialEntity, Emissive) || emittedLuminance(registry, materialEntity) <= 0.0f) return false;

    if (type != MeshRef) return true;
    return getMeshAsset(registry, registry.get(entity, MeshRef).get<Entity>("handle")) != nullptr;
}

} // namespace

const std::vector<const ComponentType*>& objectTypeOrder() {
    static const std::vector<const ComponentType*> order = ComponentType::inGroup("object");
    return order;
}

void meshPackingSystem(Registry& registry) {
    VkSmol& engine = Core::getEngine();
    const SceneGpuBuffers& buffers = registry.ctx().get<SceneGpuBuffers>();

    std::vector<GpuMesh>& meshTemplates = registry.ctx().get<MeshTemplates>().meshes;
    meshTemplates.clear();
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
            .aabbMinX = meshAabbMin.x,
            .aabbMinY = meshAabbMin.y,
            .aabbMinZ = meshAabbMin.z,
            .aabbMaxX = meshAabbMax.x,
            .aabbMaxY = meshAabbMax.y,
            .aabbMaxZ = meshAabbMax.z,
            .smoothShading = smooth ? 1u : 0u,
            .hasVertexColor = mesh->hasVertexColor() ? 1u : 0u,
        });

        vertices.insert(vertices.end(), meshVertices.begin(), meshVertices.end());
        // Offset the indices by the vertex offset
        for (size_t i = 0; i < meshIndices.size(); i++) { indices.push_back(meshIndices[i] + vertexOffset); }
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

    engine.writeBuffer(buffers.vertex, vertices);
    engine.writeBuffer(buffers.index, indices);
    engine.writeBuffer(buffers.bvh, bvhNodes);
}

void meshInstancePackingSystem(Registry& registry) {
    const std::vector<GpuMesh>& meshTemplates = registry.ctx().get<MeshTemplates>().meshes;
    auto& meshRefs = registry.storage(MeshRef);
    auto& transforms = registry.storage(Transform);

    std::vector<GpuMesh> meshes;
    const EntitySlots meshSlots = buildEntitySlots(registry.getChildren(registry.ctx().get<SceneRoots>().assetsRoot));

    for (const auto& entity : meshRefs.entities()) {
        if (!transforms.has(entity)) continue;
        const uint32_t meshSlot = resolveMeshSlot(meshSlots, meshRefs, entity);
        if (meshSlot >= static_cast<uint32_t>(meshTemplates.size())) continue;
        meshes.push_back(meshTemplates[meshSlot]);
    }

    Core::getEngine().writeBuffer(registry.ctx().get<SceneGpuBuffers>().mesh, meshes);
}

void materialPackingSystem(Registry& registry) {
    const auto& materialEntities = registry.getChildren(registry.ctx().get<SceneRoots>().materialsRoot);

    std::vector<GpuMaterial> gpuMaterials;
    std::vector<float> pluginParams;
    gpuMaterials.reserve(materialEntities.size());

    for (const ecs::Entity& entity : materialEntities) {
        GpuMaterial gpu{};
        if (!MaterialTable::pack(registry, entity, gpu, pluginParams) && !gpuMaterials.empty())
            gpu = gpuMaterials.front();
        gpuMaterials.push_back(gpu);
    }

    // Slack so unpackMaterial's fixed-size read past the last material's base never goes out of bounds.
    pluginParams.resize(pluginParams.size() + kMaterialPayloadSize, 0.0f);

    LensPluginInfo& lensInfo = registry.ctx().get<LensPluginInfo>();
    lensInfo.paramsBase = static_cast<int32_t>(pluginParams.size());
    lensInfo.slot = CameraLensTable::pack(registry, *registry.ctx().get<Entity*>(), pluginParams);

    SkyPluginInfo& skyInfo = registry.ctx().get<SkyPluginInfo>();
    const size_t skyBase = pluginParams.size();
    const bool skyActive = SkyTable::pack(registry, resolveEnvironmentEntity(registry), pluginParams);
    skyInfo.paramsBase = skyActive ? static_cast<int32_t>(skyBase) : -1;

    CompositingChainInfo& compositingInfo = registry.ctx().get<CompositingChainInfo>();
    compositingInfo.passes.clear();
    for (CompositingPassEntry& pass : CompositingTable::passes(registry)) {
        CompositingPassInfo info;
        info.paramsBase = static_cast<int32_t>(pluginParams.size());
        info.slot = CompositingTable::pack(pass, pluginParams);
        for (int p = 0; p < std::max(1, pass.plugin->getPassCount()); p++) {
            info.passId = p;
            compositingInfo.passes.push_back(info);
        }
    }

    VkSmol& engine = Core::getEngine();
    const SceneGpuBuffers& buffers = registry.ctx().get<SceneGpuBuffers>();
    engine.writeBuffer(buffers.material, gpuMaterials);
    engine.writeBuffer(buffers.pluginParams, pluginParams);
}

void objectPackingSystem(Registry& registry) {
    auto& transforms = registry.storage(Transform);
    const auto& materialRefs = registry.storage(MaterialRef);

    std::vector<GpuObject> gpuObjects;
    std::vector<GpuMotionSample> motion;
    std::vector<GpuMotionSample> liveMotion;
    std::unordered_map<Entity, int>& objectIndices = registry.ctx().get<ObjectIndices>().byEntity;
    objectIndices.clear();
    const EntitySlots materialSlots =
        buildEntitySlots(registry.getChildren(registry.ctx().get<SceneRoots>().materialsRoot));

    const Entity cameraEntity = *registry.ctx().get<Entity*>();
    const bool hasCamera = transforms.has(cameraEntity);
    const glm::vec3 cameraPosition =
        hasCamera ? transforms.get(cameraEntity).get<glm::vec3>("position") : glm::vec3(0.0f);
    std::vector<std::pair<float, int>> cameraMedia;

    int32_t lightCount = 0;
    const std::vector<const ComponentType*>& order = objectTypeOrder();
    for (size_t i = 0; i < order.size(); i++) {
        const ObjectType objectType = static_cast<ObjectType>(i + 1);
        uint32_t idx = 0;
        for (const auto& entity : registry.storage(*order[i]).entities()) {
            if (!transforms.has(entity)) continue;
            const uint32_t motionOffset = bakeMotionSamples(registry, entity, transforms.get(entity), motion);
            bakeLiveTransform(transforms.get(entity), liveMotion);
            if (hasCamera && isMediumCandidate(registry, materialRefs, entity)) {
                const float volume = containingVolume(registry, *order[i], entity,
                                                      composeTransform(transforms.get(entity)), cameraPosition);
                if (volume > 0.0f) cameraMedia.emplace_back(volume, static_cast<int>(gpuObjects.size()));
            }
            objectIndices[entity] = static_cast<int>(gpuObjects.size());
            gpuObjects.push_back(GpuObject{
                .type = objectType,
                .id = idx,
                .materialSlot = resolveMaterialSlot(materialSlots, materialRefs, entity),
                .motionOffset = motionOffset,
                .lightId = isSampledLight(registry, materialRefs, *order[i], entity) ? lightCount++ : -1,
            });
            idx++;
        }
    }

    std::ranges::sort(cameraMedia, std::ranges::greater{});
    const size_t firstMedium = cameraMedia.size() > 4 ? cameraMedia.size() - 4 : 0;
    CameraMediaInfo& cameraMediaInfo = registry.ctx().get<CameraMediaInfo>();
    cameraMediaInfo = CameraMediaInfo{};
    for (size_t i = firstMedium; i < cameraMedia.size(); i++)
        cameraMediaInfo.objects[cameraMediaInfo.count++] = cameraMedia[i].second;

    CameraMotionInfo& cameraMotion = registry.ctx().get<CameraMotionInfo>();
    if (hasCamera) {
        const uint32_t motionOffset = bakeMotionSamples(registry, cameraEntity, transforms.get(cameraEntity), motion);
        bakeLiveTransform(transforms.get(cameraEntity), liveMotion);
        cameraMotion = CameraMotionInfo{motionOffset};
    } else {
        cameraMotion = CameraMotionInfo{};
    }

    VkSmol& engine = Core::getEngine();
    const SceneGpuBuffers& buffers = registry.ctx().get<SceneGpuBuffers>();
    const GpuObjectHeader header{.objectCount = static_cast<uint32_t>(gpuObjects.size())};
    engine.writeBuffer(buffers.object, header);
    engine.writeBuffer(buffers.object, gpuObjects, sizeof(GpuObjectHeader));
    engine.writeBuffer(buffers.motion, motion);
    engine.writeBuffer(buffers.liveMotion, liveMotion);
}

void lightPackingSystem(Registry& registry) {
    auto& meshes = registry.storage(MeshRef);
    const auto& transforms = registry.storage(Transform);
    const auto& materialRefs = registry.storage(MaterialRef);

    std::vector<GpuLight> lights;
    std::vector<float> powers;
    int32_t objectId = 0;
    float totalPower = 0.0f;

    for (const ComponentType* type : objectTypeOrder()) {
        for (const auto& entity : registry.storage(*type).entities()) {
            if (!transforms.has(entity)) continue;
            objectId++;

            if (!isSampledLight(registry, materialRefs, *type, entity)) continue;

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
                const glm::vec3 scale = transforms.get(entity).get<glm::vec3>("scale");
                Component& meshRef = meshes.get(entity);
                const Entity meshAssetEntity = meshRef.get<Entity>("handle");
                const MeshAsset* meshAsset = getMeshAsset(registry, meshAssetEntity);
                if (!meshAsset) continue;

                MeshAreaCache& cache = meshRef.payload<MeshAreaCache>("area_cache");
                const uint64_t changeTick = std::max(registry.getChangeTick(Mesh), registry.getChangeTick(MeshRef));
                if (!cache.valid || cache.scale != scale || cache.changeTick != changeTick)
                    cache = MeshAreaCache{
                        .scale = scale,
                        .changeTick = changeTick,
                        .area = meshAsset->computeArea(glm::scale(glm::mat4(1.0f), scale)),
                        .valid = true,
                    };
                area = cache.area;
            } else {
                std::unreachable();
            }

            const Entity materialEntity = materialRefs.get(entity).get<Entity>("handle");
            const float power = area * emittedLuminance(registry, materialEntity);
            totalPower += power;
            powers.push_back(power);
            lights.push_back(GpuLight{
                .objectId = objectId - 1,
                .area = area,
                .selectionProbability = 0.0f,
                .cumulativeProbability = 0.0f,
            });
        }
    }

    float cumulativePower = 0.0f;
    size_t lastSelectable = 0;
    for (size_t i = 0; i < lights.size() && totalPower > 0.0f; i++) {
        cumulativePower += powers[i];
        lights[i].selectionProbability = powers[i] / totalPower;
        lights[i].cumulativeProbability = cumulativePower / totalPower;
        if (powers[i] > 0.0f) lastSelectable = i;
    }
    for (size_t i = lastSelectable; i < lights.size() && totalPower > 0.0f; i++) lights[i].cumulativeProbability = 1.0f;

    VkSmol& engine = Core::getEngine();
    const SceneGpuBuffers& buffers = registry.ctx().get<SceneGpuBuffers>();
    const GpuLightHeader header{.lightCount = static_cast<uint32_t>(lights.size())};
    engine.writeBuffer(buffers.light, header);
    engine.writeBuffer(buffers.light, lights, sizeof(GpuLightHeader));
}

} // namespace ecs
