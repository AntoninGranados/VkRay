#include "raster_renderer.hpp"

#include <cmath>
#include <string>
#include <utility>

#include "VkSmol/engine.hpp"
#include "VkSmol/graph/pass/graphics_pass_builder.hpp"
#include "VkSmol/graph/pass_context.hpp"

#include <glm/gtc/constants.hpp>

#include "core/core.hpp"
#include "core/ecs/systems/gpu_packing_system.hpp"
#include "core/render/script_table.hpp"
#include "core/scene/scene.hpp"

void RasterRenderer::init() {
    std::vector<UnitVertex> vertices;
    std::vector<uint32_t> indices;
    sphereRange = appendSphere(vertices, indices);
    boxRange = appendBox(vertices, indices);
    quadRange = appendFace(vertices, indices, glm::vec3(0.0f), glm::vec3(0.5f, 0.0f, 0.0f), glm::vec3(0.0f, 0.5f, 0.0f),
                           glm::vec3(0.0f, 0.0f, 1.0f));
    const float planeExtent = 1e4f;
    planeRange = appendFace(vertices, indices, glm::vec3(0.0f), glm::vec3(planeExtent, 0.0f, 0.0f),
                            glm::vec3(0.0f, 0.0f, planeExtent), glm::vec3(0.0f, 1.0f, 0.0f));

    VkSmol::Engine& engine = Core::getEngine();
    unitVertexHandle = engine.createBuffer("RasterUnitVertices", vertices.size() * sizeof(UnitVertex), vertices.data());
    unitIndexHandle = engine.createBuffer("RasterUnitIndices", indices.size() * sizeof(uint32_t), indices.data());
}

void RasterRenderer::addPasses(VkSmol::RenderGraph& graph, const SceneGpuBuffers& sceneHandles,
                               VkSmol::ImageHandle colorImage, VkSmol::ImageHandle depthImage, bool shaded) {
    const std::string shaderPath = "builtin:/shaders/editor/raster.slang";
    std::vector<std::pair<VkShaderStageFlagBits, std::string>> stages = {{VK_SHADER_STAGE_VERTEX_BIT, shaderPath}};
    if (shaded) stages.emplace_back(VK_SHADER_STAGE_FRAGMENT_BIT, shaderPath);

    VkSmol::GraphicsPassBuilder pass = graph.addGraphicsPass(kPassName);
    pass.pipeline(stages, {},
                  {.primitive = {.cullMode = VK_CULL_MODE_NONE},
                   .depth = VkSmol::DepthState{.compare = VK_COMPARE_OP_GREATER},
                   .blend = {}},
                  ScriptTable::conformances({&ScriptTable::kMaterial}))
        .read("unitVertices", unitVertexHandle)
        .read("unitIndices", unitIndexHandle)
        .read("scene.vertices", sceneHandles.vertex)
        .read("scene.indices", sceneHandles.index)
        .read("scene.meshes", sceneHandles.mesh)
        .read("scene.materials", sceneHandles.material)
        .read("scene.pluginParams", sceneHandles.pluginParams)
        .read("scene.objects", sceneHandles.object)
        .read("scene.motion", sceneHandles.liveMotion)
        .depthAttachment(depthImage, VkSmol::AttachmentLoad::Clear, 0.0f)
        .execute([this](VkSmol::PassContext& context) {
            for (const Draw& draw : draws) {
                constants.objectIndex = draw.objectIndex;
                constants.firstIndex = draw.indices.first;
                context.pushConstants(constants);
                context.draw(draw.indices.count);
            }
        });
    if (shaded) pass.colorAttachment(colorImage, VkSmol::AttachmentLoad::Clear, {0.05f, 0.05f, 0.05f, 1.0f});
}

void RasterRenderer::prepare(const Scene& scene, const glm::mat4& viewProjection, const glm::vec3& cameraPosition) {
    constants.viewProjection = viewProjection;
    constants.cameraPosition = cameraPosition;

    draws.clear();
    const PackingInfo& packingInfo = scene.getPackingInfo();
    for (uint32_t objectIndex = 0; objectIndex < packingInfo.objects.size(); ++objectIndex) {
        const GpuObject& object = packingInfo.objects[objectIndex];
        IndexRange indices = unitRange(object.type);
        if (object.type == ObjectType::Mesh && object.id < packingInfo.meshInstances.size()) {
            const GpuMesh& mesh = packingInfo.meshInstances[object.id];
            indices = {mesh.indexOffset, mesh.triangleCount * 3};
        }
        if (indices.count > 0) draws.push_back({objectIndex, indices});
    }
}

RasterRenderer::IndexRange RasterRenderer::unitRange(ObjectType type) const {
    switch (type) {
    case ObjectType::Sphere:
        return sphereRange;
    case ObjectType::Box:
        return boxRange;
    case ObjectType::Quad:
        return quadRange;
    case ObjectType::Plane:
        return planeRange;
    default:
        return {};
    }
}

RasterRenderer::IndexRange RasterRenderer::appendSphere(std::vector<UnitVertex>& vertices,
                                                        std::vector<uint32_t>& indices) {
    const uint32_t rings = 32;
    const uint32_t segments = 64;
    const uint32_t base = static_cast<uint32_t>(vertices.size());
    const IndexRange range{static_cast<uint32_t>(indices.size()), rings * segments * 6};
    for (uint32_t ring = 0; ring <= rings; ++ring) {
        const float theta = glm::pi<float>() * static_cast<float>(ring) / static_cast<float>(rings);
        for (uint32_t segment = 0; segment <= segments; ++segment) {
            const float phi = glm::two_pi<float>() * static_cast<float>(segment) / static_cast<float>(segments);
            const glm::vec3 position(std::sin(theta) * std::cos(phi), std::cos(theta), std::sin(theta) * std::sin(phi));
            vertices.push_back({position, position});
        }
    }
    for (uint32_t ring = 0; ring < rings; ++ring)
        for (uint32_t segment = 0; segment < segments; ++segment) {
            const uint32_t first = base + ring * (segments + 1) + segment;
            const uint32_t below = first + segments + 1;
            indices.insert(indices.end(), {first, below, first + 1, first + 1, below, below + 1});
        }
    return range;
}

RasterRenderer::IndexRange RasterRenderer::appendBox(std::vector<UnitVertex>& vertices,
                                                     std::vector<uint32_t>& indices) {
    const IndexRange range{static_cast<uint32_t>(indices.size()), 36};
    for (int axis = 0; axis < 3; ++axis)
        for (const float side : {-1.0f, 1.0f}) {
            glm::vec3 normal(0.0f);
            glm::vec3 u(0.0f);
            glm::vec3 v(0.0f);
            normal[axis] = side;
            u[(axis + 1) % 3] = 1.0f;
            v[(axis + 2) % 3] = 1.0f;
            appendFace(vertices, indices, normal, u, v, normal);
        }
    return range;
}

RasterRenderer::IndexRange RasterRenderer::appendFace(std::vector<UnitVertex>& vertices, std::vector<uint32_t>& indices,
                                                      const glm::vec3& center, const glm::vec3& u, const glm::vec3& v,
                                                      const glm::vec3& normal) {
    const uint32_t base = static_cast<uint32_t>(vertices.size());
    const IndexRange range{static_cast<uint32_t>(indices.size()), 6};
    vertices.push_back({center - u - v, normal});
    vertices.push_back({center + u - v, normal});
    vertices.push_back({center + u + v, normal});
    vertices.push_back({center - u + v, normal});
    indices.insert(indices.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
    return range;
}
