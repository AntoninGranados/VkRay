#pragma once

#include <cstdint>
#include <vector>

#include "VkSmol/graph/builder_resource.hpp"
#include "VkSmol/graph/render_graph.hpp"

#include <glm/glm.hpp>

#include "core/scene/gpu_structs.hpp"
#include "editor/render_structures.hpp"

class Scene;
struct SceneGpuBuffers;

class RasterRenderer {
public:
    static constexpr const char* kPassName = "RasterPass";

    void init();
    void addPasses(VkSmol::RenderGraph& graph, const SceneGpuBuffers& sceneHandles, VkSmol::ImageHandle colorImage,
                   VkSmol::ImageHandle depthImage, bool shaded);
    void prepare(const Scene& scene, const glm::mat4& viewProjection, const glm::vec3& cameraPosition);

private:
    struct IndexRange {
        uint32_t first = 0;
        uint32_t count = 0;
    };

    struct Draw {
        uint32_t objectIndex;
        IndexRange indices;
    };

    static IndexRange appendSphere(std::vector<UnitVertex>& vertices, std::vector<uint32_t>& indices);
    static IndexRange appendBox(std::vector<UnitVertex>& vertices, std::vector<uint32_t>& indices);
    static IndexRange appendFace(std::vector<UnitVertex>& vertices, std::vector<uint32_t>& indices,
                                 const glm::vec3& center, const glm::vec3& u, const glm::vec3& v,
                                 const glm::vec3& normal);
    IndexRange unitRange(ObjectType type) const;

    IndexRange sphereRange;
    IndexRange boxRange;
    IndexRange quadRange;
    IndexRange planeRange;
    std::vector<Draw> draws;
    RasterConstants constants{};
    VkSmol::BufferHandle unitVertexHandle;
    VkSmol::BufferHandle unitIndexHandle;
};
