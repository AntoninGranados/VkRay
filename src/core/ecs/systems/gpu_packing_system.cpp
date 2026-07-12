#include "gpu_packing_system.hpp"

#include <cstring>
#include <vector>

#include "VkSmol/engine.hpp"
#include "VkSmol/frame_context.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include "core/scene/object/object.hpp"
#include "core/core.hpp"
#include "core/scene/scene.hpp"

namespace ecs {

static size_t sceneCapacityFromCount(size_t count) {
    size_t cap = 16;
    while (cap < count) cap <<= 1;
    return cap;
}

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
    auto& spheres = registry.storage<ecs::Sphere>();
    auto& transforms = registry.storage<ecs::Transform>();
    auto& materialRefs = registry.storage<ecs::MaterialRef>();
    auto& packingMaps = Core::getScene().getPackingMaps();

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

    fillBufferWithPadding(frame, Core::getScene().getBuffers().sphere, gpuSpheres);
}

void planePackingSystem(Registry& registry, const FrameContext& frame) {
    auto& planes = registry.storage<ecs::Plane>();
    auto& transforms = registry.storage<ecs::Transform>();
    auto& materialRefs = registry.storage<ecs::MaterialRef>();
    auto& packingMaps = Core::getScene().getPackingMaps();

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

    fillBufferWithPadding(frame, Core::getScene().getBuffers().plane, gpuPlanes);
}

void boxPackingSystem(Registry& registry, const FrameContext& frame) {
    auto& boxes = registry.storage<ecs::Box>();
    auto& transforms = registry.storage<ecs::Transform>();
    auto& materialRefs = registry.storage<ecs::MaterialRef>();
    auto& packingMaps = Core::getScene().getPackingMaps();

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

    fillBufferWithPadding(frame, Core::getScene().getBuffers().box, gpuBoxes);
}

void quadPackingSystem(Registry& registry, const FrameContext& frame) {
    auto& quads = registry.storage<ecs::Quad>();
    auto& transforms = registry.storage<ecs::Transform>();
    auto& materialRefs = registry.storage<ecs::MaterialRef>();
    auto& packingMaps = Core::getScene().getPackingMaps();

    std::vector<GpuQuad> gpuQuads;
    packingMaps.quadId.clear();
    size_t quadId = 0;

    for (const auto& e : quads.entities()) {
        if (!transforms.has(e)) continue;
        ecs::Transform& t = transforms.get(e);

        MaterialHandle handle = materialRefs.has(e) ? materialRefs.get(e).handle : 0;

        const ecs::Quad& q = quads.get(e);
        gpuQuads.push_back(GpuQuad{
            .point  = t.position,
            .u      = q.u,
            .v      = q.v,
            .normal = q.normal,
            .materialHandle = handle,
        });

        packingMaps.quadId[e] = quadId++;
    }

    fillBufferWithPadding(frame, Core::getScene().getBuffers().quad, gpuQuads);
}

void meshPackingSystem(Registry& registry, const FrameContext& frame) {
    auto& meshRefs = registry.storage<ecs::MeshRef>();
    auto& transforms = registry.storage<ecs::Transform>();
    auto& materialRefs = registry.storage<ecs::MaterialRef>();
    auto& packingMaps = Core::getScene().getPackingMaps();

    std::vector<GpuMesh> meshTemplates;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<GpuBvhNode> bvhNodes;

    for (const auto& mesh : Core::getScene().getMeshAssets()) {
        auto& meshVertices = mesh.getVertices();
        auto& meshIndices = mesh.getIndices();
        auto& meshBvhNodes = mesh.getBvhNodes();

        uint32_t vertexOffset = static_cast<uint32_t>(vertices.size());
        uint32_t indexOffset = static_cast<uint32_t>(indices.size());
        uint32_t bvhOffset = static_cast<uint32_t>(bvhNodes.size());
        glm::vec3 meshAabbMin = mesh.getAabbMin();
        glm::vec3 meshAabbMax = mesh.getAabbMax();
        meshTemplates.push_back(GpuMesh{
            .indexOffset   = indexOffset,
            .triangleCount = static_cast<uint32_t>(meshIndices.size() / 3),
            .bvhOffset     = bvhOffset,
            .bvhNodeCount  = static_cast<uint32_t>(meshBvhNodes.size()),
            .aabbMinX = meshAabbMin.x, .aabbMinY = meshAabbMin.y, .aabbMinZ = meshAabbMin.z,
            .aabbMaxX = meshAabbMax.x, .aabbMaxY = meshAabbMax.y, .aabbMaxZ = meshAabbMax.z,
            .smoothShading   = mesh.getSmoothShading() ? 1u : 0u,
            .hasVertexColor  = mesh.hasVertexColor()   ? 1u : 0u,
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
    packingMaps.meshId.clear();
    size_t meshId = 0;

    for (const auto& e : meshRefs.entities()) {
        if (!transforms.has(e)) continue;
        ecs::MeshRef& meshRef = meshRefs.get(e);
        const GpuMesh& meshTemplate = meshTemplates[meshRef.handle];
        ecs::Transform& t = transforms.get(e);

        MaterialHandle handle = materialRefs.has(e) ? materialRefs.get(e).handle : 0;

        meshes.push_back(GpuMesh{
            .transform      = t.local,
            .invTransform   = glm::inverse(t.local),
            .indexOffset    = meshTemplate.indexOffset,
            .triangleCount  = meshTemplate.triangleCount,
            .bvhOffset      = meshTemplate.bvhOffset,
            .bvhNodeCount   = meshTemplate.bvhNodeCount,
            .aabbMinX = meshTemplate.aabbMinX, .aabbMinY = meshTemplate.aabbMinY, .aabbMinZ = meshTemplate.aabbMinZ,
            .aabbMaxX = meshTemplate.aabbMaxX, .aabbMaxY = meshTemplate.aabbMaxY, .aabbMaxZ = meshTemplate.aabbMaxZ,
            .materialHandle = handle,
            .smoothShading  = meshTemplate.smoothShading,
            .hasVertexColor = meshTemplate.hasVertexColor,
        });

        packingMaps.meshId[e] = meshId++;
    }

    fillBufferWithPadding(frame, Core::getScene().getBuffers().mesh, meshes);
}

void materialPackingSystem(Registry&, const FrameContext& frame) {
    std::vector<GpuMaterial> materials;

    for (const auto& mat : Core::getScene().getMaterials()) {
        materials.push_back(GpuMaterial{
            .type = mat.type,
            .albedo = mat.albedo,
            .roughness = mat.roughness,
            .metalness = mat.metalness,
            .ior = mat.ior,
            .transmission = mat.transmission,
            .emissionStrength = mat.emissionStrength,
            .density = mat.density,
            .anisotropic = mat.anisotropic,
        });
    }

    fillBufferWithPadding(frame, Core::getScene().getBuffers().material, materials);
}

void objectPackingSystem(Registry& registry, const FrameContext& frame) {
    const auto& packingMaps = Core::getScene().getPackingMaps();

    std::vector<ObjectHandle> objectHandles;

    // Spheres
    for (const auto& [e, id] : packingMaps.sphereId) {
        objectHandles.push_back(ObjectHandle{ .type = ObjectType::Sphere, .id = id });
    }
    // Planes
    for (const auto& [e, id] : packingMaps.planeId) {
        objectHandles.push_back(ObjectHandle{ .type = ObjectType::Plane, .id = id });
    }
    // Boxes
    for (const auto& [e, id] : packingMaps.boxId) {
        objectHandles.push_back(ObjectHandle{ .type = ObjectType::Box, .id = id });
    }
    // Quads
    for (const auto& [e, id] : packingMaps.quadId) {
        objectHandles.push_back(ObjectHandle{ .type = ObjectType::Quad, .id = id });
    }
    // Meshes
    for (const auto& [e, id] : packingMaps.meshId) {
        objectHandles.push_back(ObjectHandle{ .type = ObjectType::Mesh, .id = id });
    }

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
    const auto& spheres = registry.storage<ecs::Sphere>();
    const auto& meshRefs = registry.storage<ecs::MeshRef>();
    const auto& transforms = registry.storage<ecs::Transform>();
    const auto& materialRefs = registry.storage<ecs::MaterialRef>();
    const auto& materials = Core::getScene().getMaterials();
    const auto& meshAssets = Core::getScene().getMeshAssets();
    const auto& packingMaps = Core::getScene().getPackingMaps();

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
    // Quads
    for (const auto& [e, id] : packingMaps.quadId) {
        objectId++;
        if (!materialRefs.has(e) || materials[materialRefs.get(e).handle].type != MaterialType::Emissive) continue;

        const glm::mat4& local = transforms.get(e).local;
        const glm::vec3 u = glm::vec3(local[0]);
        const glm::vec3 v = glm::vec3(local[2]);
        const float area = glm::length(glm::cross(u, v));
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
