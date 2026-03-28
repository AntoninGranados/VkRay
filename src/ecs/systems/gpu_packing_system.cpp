#include "gpu_packing_system.hpp"

#include <vector>

#include <glm/gtc/matrix_transform.hpp>

#include "engine/engine.hpp"
#include "scene/scene.hpp"
#include "scene/object/object.hpp"
#include "app/app_context.hpp"

namespace ecs {

// TODO: should pass FrameContext here
template <typename T, typename Header = std::monostate>
inline void fillBufferWithPadding(AppContext& ctx, DynamicPerFrameBuffer<T, Header>& buffer, std::vector<T>& data) {
    if (ctx.engine->setDynamicPerFrameBufferCount(buffer, data.size())) {
        ctx.scene->markBufferUpdated();
    }

    std::vector<T> padded = data;
    padded.resize(buffer.getCapacity());
    ctx.engine->fillBuffer(buffer.at(ctx.engine->getFrame()), padded.data());
}

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

    fillBufferWithPadding(ctx, ctx.scene->getSphereBuffers(), gpuSpheres);
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
        // ecs::Plane& p = planes.get(e);
        ecs::Transform& t = transforms.get(e);

        MaterialHandle handle = materialRefs.has(e) ? materialRefs.get(e).handle : 0;
        
        gpuPlanes.push_back(GpuPlane{
            .point = t.position,
            .normal = glm::normalize(t.rotation * glm::vec3(0.0f, 1.0f, 0.0f)),
            .materialHandle = handle,
        });
        
        packingMaps.planeId[e] = planeId++;
    }

    fillBufferWithPadding(ctx, ctx.scene->getPlaneBuffers(), gpuPlanes);
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
        // ecs::Box& b = boxes.get(e);
        ecs::Transform& t = transforms.get(e);

        MaterialHandle handle = materialRefs.has(e) ? materialRefs.get(e).handle : 0;
        
        gpuBoxes.push_back(GpuBox{
            .transform = t.local,
            .invTransform = glm::inverse(t.local),
            .materialHandle = handle,
        });

        packingMaps.boxId[e] = boxId++;
    }

    fillBufferWithPadding(ctx, ctx.scene->getBoxBuffers(), gpuBoxes);
}

void meshPackingSystem(Registry& registry, AppContext& ctx) {
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

    fillBufferWithPadding(ctx, ctx.scene->getVertexBuffers(), vertices);
    fillBufferWithPadding(ctx, ctx.scene->getIndexBuffers(), indices);
    fillBufferWithPadding(ctx, ctx.scene->getBvhBuffers(), bvhNodes);

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

    fillBufferWithPadding(ctx, ctx.scene->getMeshBuffers(), meshes);
}

void materialPackingSystem(Registry& registry, AppContext& ctx) {
    std::vector<GpuMaterial> materials;

    for (const auto& mat : ctx.scene->getMaterials()) {
        materials.push_back(GpuMaterial{
            .type = mat.type,
            .albedo = mat.albedo,
            .payload = { mat.payload[0], mat.payload[1] }
        });
    }

    fillBufferWithPadding(ctx, ctx.scene->getMaterialBuffers(), materials);
}

void objectPackingSystem(Registry& registry, AppContext& ctx) {
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

    auto& objectBuffers = ctx.scene->getObjectBuffers();
    if (ctx.engine->setDynamicPerFrameBufferCount(objectBuffers, objectHandles.size())) {
        ctx.scene->markBufferUpdated();
    }
    uint32_t objectCount = static_cast<uint32_t>(objectHandles.size());
    objectHandles.resize(objectBuffers.getCapacity());

    std::vector<char> objectData(sizeof(GpuObjectHeader) + sizeof(ObjectHandle) * objectBuffers.getCapacity(), 0);
    size_t offset = 0;
    // Compute the object buffers data (including the header)
    memcpy(objectData.data() + offset, &objectCount, sizeof(objectCount));
    offset += sizeof(objectCount);
    memcpy(objectData.data() + offset, &objectSelected, sizeof(objectSelected));
    offset += sizeof(objectSelected);
    memcpy(objectData.data() + offset, objectHandles.data(), objectHandles.size() * sizeof(ObjectHandle));

    ctx.engine->fillBuffer(objectBuffers.at(ctx.engine->getFrame()), objectData.data());
}

void lightPackingSystem(Registry& registry, AppContext& ctx) {
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

    auto& lightBuffers = ctx.scene->getLightBuffers();
    if (ctx.engine->setDynamicPerFrameBufferCount(lightBuffers, lights.size())) {
        ctx.scene->markBufferUpdated();
    }
    lights.resize(lightBuffers.getCapacity());

    std::vector<char> lightData(sizeof(GpuLightHeader) + sizeof(GpuLight) * lightBuffers.getCapacity(), 0);
    // Compute the light buffers data (including the header)
    size_t offset = 0;
    memcpy(lightData.data() + offset, &totalArea, sizeof(totalArea));
    offset += sizeof(totalArea);
    memcpy(lightData.data() + offset, lights.data(), lights.size() * sizeof(GpuLight));
    
    ctx.engine->fillBuffer(lightBuffers.at(ctx.engine->getFrame()), lightData.data());
}

} // namespace ecs
