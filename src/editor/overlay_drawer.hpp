#pragma once

#include <cstdint>
#include <vector>

#include "VkSmol/graph/builder_resource.hpp"
#include "VkSmol/graph/render_graph.hpp"

#include <glm/glm.hpp>

#include "editor/render_structures.hpp"

class OverlayDrawer {
public:
    static constexpr const char* kLinesPassName = "OverlayLinesPass";

    void init();
    void addPasses(VkSmol::RenderGraph& graph, VkSmol::ImageHandle colorImage, VkSmol::ImageHandle depthImage);
    void upload(const glm::mat4& viewProjection, VkExtent2D viewportExtent, float lineWidth);

    void line(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color);

private:
    std::vector<OverlayLine> lines;
    uint32_t lineCount = 0;
    OverlayConstants constants{};
    VkSmol::BufferHandle linesHandle;
};
