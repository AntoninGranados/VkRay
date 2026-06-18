#include "gpu_packing_system.hpp"

#include <vector>

#include <glm/gtc/matrix_transform.hpp>

#include "engine/engine.hpp"
#include "engine/frame_context.hpp"
#include "scene/scene.hpp"
#include "scene/object/object.hpp"
#include "app/app_context.hpp"

namespace ecs {

static size_t sceneCapacityFromCount(size_t count) {
    size_t cap = 16;
    while (cap < count) cap <<= 1;
    return cap;
}

template <typename T>
inline void fillBufferWithPadding(AppContext& ctx, const FrameContext& frame, SceneGpuBufferEntry& entry, std::vector<T>& data) {
    size_t required = sceneCapacityFromCount(data.size());
    if (required > entry.capacity) {
        ctx.engine->resizeBuffer(entry.handle, required * sizeof(T));
        entry.capacity = required;
    }
    data.resize(entry.capacity);
    ctx.engine->fillBuffer(ctx.engine->getBuffer(entry.handle, frame.currentFrame), data.data());
}

void spherePackingSystem(Registry& registry, AppContext& ctx, const FrameContext& frame) {
    auto& spheres = registry.storage<ecs::Sphere>();
    auto& transforms = registry.storage<ecs::Transform>();
    auto& materialRefs = registry.storage<ecs::MaterialRef>();
    auto& packingMaps = ctx.scene->getPackingMaps();
    
    std::vector<GpuSphere> gpuSpheres;
    packingMaps.sphereId.clear();
    size_t sphereId = 0;

    for (const auto& e : spheres.entities()) {
        if (!transforms.has(e)) continue;
        ecs::Sphere& s = spheres.get(e);
        ecs::Transform& t = transforms.get(e);

        MaterialHandle handle = materialRefs.has(e) ? materialRefs.get(e).handle : 0;
        
        gpuSpheres.push_back(GpuSphere{
            .center = t.position,
            .radius = s.radius,
            .materialHandle = handle,
        });
                
        packingMaps.sphereId[e] = sphereId++;
    }

    fillBufferWithPadding(ctx, frame, ctx.scene->getBuffers().sphere, gpuSpheres);
}

void planePackingSystem(Registry& registry, AppContext& ctx, const FrameContext& frame) {
    auto& planes = registry.storage<ecs::Plane>();
    auto& transforms = registry.storage<ecs::Transform>();
    auto& materialRefs = registry.storage<ecs::MaterialRef>();
    auto& packingMaps = ctx.scene->getPackingMaps();

    std::vector<GpuPlane> gpuPlanes;
    packingMaps.planeId.clear();
    size_t planeId = 0;

    for (const auto& e : planes.entities()) {
        if (!transforms.has(e)) continue;
        ecs::Transform& t = transforms.get(e);

        MaterialHandle handle = materialRefs.has(e) ? materialRefs.get(e).handle : 0;
        
        gpuPlanes.push_back(GpuPlane{
            .point = t.position,
            .normal = glm::normalize(t.rotation * glm::vec3(0.0f, 1.0f, 0.0f)),
            .materialHandle = handle,
        });
        
        packingMaps.planeId[e] = planeId++;
    }

    fillBufferWithPadding(ctx, frame, ctx.scene->getBuffers().plane, gpuPlanes);
}

void boxPackingSystem(Registry& registry, AppContext& ctx, const FrameContext& frame) {
    auto& boxes = registry.storage<ecs::Box>();
    auto& transforms = registry.storage<ecs::Transform>();
    auto& materialRefs = registry.storage<ecs::MaterialRef>();
    auto& packingMaps = ctx.scene->getPackingMaps();

    std::vector<GpuBox> gpuBoxes;
    packingMaps.boxId.clear();
    size_t boxId = 0;

    for (const auto& e : boxes.entities()) {
        if (!transforms.has(e)) continue;
        ecs::Transform& t = transforms.get(e);

        MaterialHandle handle = materialRefs.has(e) ? materialRefs.get(e).handle : 0;

        gpuBoxes.push_back(GpuBox{
            .transform = t.local,
            .invTransform = glm::inverse(t.local),
            .materialHandle = handle,
        });

        packingMaps.boxId[e] = boxId++;
    }

    fillBufferWithPadding(ctx, frame, ctx.scene->getBuffers().box, gpuBoxes);
}

void meshPackingSystem(Registry& registry, AppContext& ctx, const FrameContext& frame) {
    auto& meshRefs = registry.storage<ecs::MeshRef>();
    auto& transforms = registry.storage<ecs::Transform>();
    auto& materialRefs = registry.storage<ecs::MaterialRef>();
    auto& packingMaps = ctx.scene->getPackingMaps();

    std::vector<GpuMesh> meshTemplates;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<GpuBvhNode> bvhNodes;

    for (const auto& mesh : ctx.scene->getMeshAssets()) {
        auto& meshVertices = mesh.getVertices();
        auto& meshIndices = mesh.getIndices();
        auto& meshBvhNodes = mesh.getBvhNodes();

        uint32_t vertexOffset = static_cast<uint32_t>(vertices.size());
        uint32_t indexOffset = static_cast<uint32_t>(indices.size());
        uint32_t bvhOffset = static_cast<uint32_t>(bvhNodes.size());
        meshTemplates.push_back(GpuMesh{
            .indexOffset = indexOffset,
            .triangleCount = static_cast<uint32_t>(meshIndices.size() / 3),
            .bvhOffset = bvhOffset,
            .bvhNodeCount = static_cast<uint32_t>(meshBvhNodes.size()),
        });

        vertices.insert(vertices.end(), meshVertices.begin(), meshVertices.end());
        // Offset the indices by the vertex offset
        for (size_t i = 0; i < meshIndices.size(); i++) {
            indices.push_back(meshIndices[i] + vertexOffset);
        }
        // Offset the BVH links and leaf links
        for (size_t n = 0; n < meshBvhNodes.size(); n++) {
            const GpuBvhNode& node = meshBvhNodes[n];
            uint32_t data0, data1;
            if (node.isLeaf != 0) {
                data0 = static_cast<uint32_t>(node.data0 + (indexOffset / 3));
                data1 = node.data1;
            } else {
                data0 = static_cast<uint32_t>(node.data0 + bvhOffset);
                data1 = static_cast<uint32_t>(node.data1 + bvhOffset);
            }
            bvhNodes.push_back(GpuBvhNode{
                .aabbMin = node.aabbMin,
                .aabbMax = node.aabbMax,
                .data0 = data0,
                .data1 = data1,
                .isLeaf = node.isLeaf,
            });
        }
    }

    fillBufferWithPadding(ctx, frame, ctx.scene->getBuffers().vertex, vertices);
    fillBufferWithPadding(ctx, frame, ctx.scene->getBuffers().index, indices);
    fillBufferWithPadding(ctx, frame, ctx.scene->getBuffers().bvh, bvhNodes);

    std::vector<GpuMesh> meshes;
    packingMaps.meshId.clear();
    size_t meshId = 0;

    for (const auto& e : meshRefs.entities()) {
        if (!transforms.has(e)) continue;
        ecs::MeshRef& meshRef = meshRefs.get(e);
        const GpuMesh& meshTemplate = meshTemplates[meshRef.handle];
        ecs::Transform& t = transforms.get(e);

        MaterialHandle handle = materialRefs.has(e) ? materialRefs.get(e).handle : 0;

        meshes.push_back(GpuMesh{
            .transform = t.local,
            .invTransform = glm::inverse(t.local),
            .indexOffset = meshTemplate.indexOffset,
            .triangleCount = meshTemplate.triangleCount,
            .bvhOffset = meshTemplate.bvhOffset,
            .bvhNodeCount = meshTemplate.bvhNodeCount,
            .materialHandle = handle,
        });

        packingMaps.meshId[e] = meshId++;
    }

    fillBufferWithPadding(ctx, frame, ctx.scene->getBuffers().mesh, meshes);
}

void materialPackingSystem(Registry&, AppContext& ctx, const FrameContext& frame) {
    std::vector<GpuMaterial> materials;

    for (const auto& mat : ctx.scene->getMaterials()) {
        materials.push_back(GpuMaterial{
            .type = mat.type,
            .albedo = mat.albedo,
            .roughness = mat.roughness,
            .ior = mat.ior,
            .emissionStrength = mat.emissionStrength,
        });
    }

    fillBufferWithPadding(ctx, frame, ctx.scene->getBuffers().material, materials);
}

void objectPackingSystem(Registry& registry, AppContext& ctx, const FrameContext& frame) {
    const auto& packingMaps = ctx.scene->getPackingMaps();

    std::vector<ObjectHandle> objectHandles;
    const ecs::Entity* selectedEntity = ctx.scene->getSelectedEntity();
    int32_t objectSelected = -1;
    int32_t objectId = 0;

    // Spheres
    for (const auto& [e, id] : packingMaps.sphereId) {
        objectHandles.push_back(ObjectHandle{
            .type = ObjectType::Sphere,
            .id = id,
        });
        if (selectedEntity && e == *selectedEntity) objectSelected = objectId;
        objectId++;
    }

    // Planes
    for (const auto& [e, id] : packingMaps.planeId) {
        objectHandles.push_back(ObjectHandle{
            .type = ObjectType::Plane,
            .id = id,
        });
        if (selectedEntity && e == *selectedEntity) objectSelected = objectId;
        objectId++;
    }
    // Boxes
    for (const auto& [e, id] : packingMaps.boxId) {
        objectHandles.push_back(ObjectHandle{
            .type = ObjectType::Box,
            .id = id,
        });
        if (selectedEntity && e == *selectedEntity) objectSelected = objectId;
        objectId++;
    }
    // Meshes
    for (const auto& [e, id] : packingMaps.meshId) {
        objectHandles.push_back(ObjectHandle{
            .type = ObjectType::Mesh,
            .id = id,
        });
        if (selectedEntity && e == *selectedEntity) objectSelected = objectId;
        objectId++;
    }

    SceneGpuBufferEntry& objectEntry = ctx.scene->getBuffers().object;
    size_t objectRequired = sceneCapacityFromCount(objectHandles.size());
    if (objectRequired > objectEntry.capacity) {
        ctx.engine->resizeBuffer(objectEntry.handle, sizeof(GpuObjectHeader) + objectRequired * sizeof(ObjectHandle));
        objectEntry.capacity = objectRequired;
    }
    uint32_t objectCount = static_cast<uint32_t>(objectHandles.size());
    objectHandles.resize(objectEntry.capacity);

    std::vector<char> objectData(sizeof(GpuObjectHeader) + sizeof(ObjectHandle) * objectEntry.capacity, 0);
    size_t offset = 0;
    memcpy(objectData.data() + offset, &objectCount, sizeof(objectCount));
    offset += sizeof(objectCount);
    memcpy(objectData.data() + offset, &objectSelected, sizeof(objectSelected));
    offset += sizeof(objectSelected);
    memcpy(objectData.data() + offset, objectHandles.data(), objectHandles.size() * sizeof(ObjectHandle));

    ctx.engine->fillBuffer(ctx.engine->getBuffer(objectEntry.handle, frame.currentFrame), objectData.data());
}

void lightPackingSystem(Registry& registry, AppContext& ctx, const FrameContext& frame) {
    const auto& spheres = registry.storage<ecs::Sphere>();
    const auto& meshRefs = registry.storage<ecs::MeshRef>();
    const auto& transforms = registry.storage<ecs::Transform>();
    const auto& materialRefs = registry.storage<ecs::MaterialRef>();
    const auto& materials = ctx.scene->getMaterials();
    const auto& meshAssets = ctx.scene->getMeshAssets();
    const auto& packingMaps = ctx.scene->getPackingMaps();

    std::vector<GpuLight> lights;
    int32_t objectId = 0;
    float totalArea = 0.0f;

    // Spheres
    for (const auto& [e, _] : packingMaps.sphereId) {
        objectId++;
        if (!materialRefs.has(e) || materials[materialRefs.get(e).handle].type != MaterialType::Emissive) continue;

        const float area = 4.0f * glm::pi<float>() * std::pow(spheres.get(e).radius, 2.0f);
        totalArea += area;
        lights.push_back(GpuLight{
            .objectId = objectId-1,
            .area = area,
            .pdfA = 1.0f / area,
        });
    }
    // Planes can't be used for importance sampling (infinite area)
    objectId += packingMaps.planeId.size();
    // Boxes
    for (const auto& [e, _] : packingMaps.boxId) {
        objectId++;
        if (!materialRefs.has(e) || materials[materialRefs.get(e).handle].type != MaterialType::Emissive) continue;

        const glm::mat4& local = transforms.get(e).local;
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
    // Meshes
    for (const auto& [e, _] : packingMaps.meshId) {
        objectId++;
        if (!materialRefs.has(e) || materials[materialRefs.get(e).handle].type != MaterialType::Emissive) continue;

        const glm::mat4& t = transforms.get(e).local;
        const float area = meshAssets[meshRefs.get(e).handle].computeArea(t);
        totalArea += area;
        lights.push_back(GpuLight{
            .objectId = objectId-1,
            .area = area,
            .pdfA = 1.0f / area,
        });
    }

    SceneGpuBufferEntry& lightEntry = ctx.scene->getBuffers().light;
    size_t lightRequired = sceneCapacityFromCount(lights.size());
    if (lightRequired > lightEntry.capacity) {
        ctx.engine->resizeBuffer(lightEntry.handle, sizeof(GpuLightHeader) + lightRequired * sizeof(GpuLight));
        lightEntry.capacity = lightRequired;
    }
    lights.resize(lightEntry.capacity);

    std::vector<char> lightData(sizeof(GpuLightHeader) + sizeof(GpuLight) * lightEntry.capacity, 0);
    size_t offset = 0;
    memcpy(lightData.data() + offset, &totalArea, sizeof(totalArea));
    offset += sizeof(totalArea);
    memcpy(lightData.data() + offset, lights.data(), lights.size() * sizeof(GpuLight));

    ctx.engine->fillBuffer(ctx.engine->getBuffer(lightEntry.handle, frame.currentFrame), lightData.data());
}

} // namespace ecs
