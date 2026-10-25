#include "overlay_drawer.hpp"

#include "VkSmol/engine.hpp"
#include "VkSmol/graph/pass/graphics_pass_builder.hpp"
#include "VkSmol/graph/pass_context.hpp"

#include "core/core.hpp"

void OverlayDrawer::init() { linesHandle = Core::getEngine().createBuffer("OverlayLines", sizeof(OverlayLine)); }

void OverlayDrawer::addPasses(VkSmol::RenderGraph& graph, VkSmol::ImageHandle colorImage,
                              VkSmol::ImageHandle depthImage) {
    graph.addGraphicsPass(kLinesPassName)
        .pipeline("builtin:/shaders/editor/overlay/line.slang", "builtin:/shaders/editor/overlay/line.slang", {},
                  {.primitive = {.cullMode = VK_CULL_MODE_NONE},
                   .depth = std::nullopt,
                   .blend = {VkSmol::BlendState::alpha()}})
        .read("lines", linesHandle)
        .read("depthTex", depthImage)
        .colorAttachment(colorImage, VkSmol::AttachmentLoad::Load)
        .execute([this](VkSmol::PassContext& context) {
            if (lineCount == 0) return;
            context.pushConstants(constants);
            context.draw(lineCount * 6);
        });
}

void OverlayDrawer::upload(const glm::mat4& viewProjection, VkExtent2D viewportExtent, float lineWidth) {
    constants = {viewProjection, glm::vec2(viewportExtent.width, viewportExtent.height), lineWidth};
    lineCount = static_cast<uint32_t>(lines.size());

    VkSmol::Engine& engine = Core::getEngine();
    const size_t size = lines.size() * sizeof(OverlayLine);
    if (engine.getBufferSize(linesHandle) < size) engine.resizeBuffer(linesHandle, size);
    if (!lines.empty()) engine.writeBuffer(linesHandle, lines);
    lines.clear();
}

void OverlayDrawer::line(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color, bool onTop) {
    lines.push_back({start, end, color, onTop ? 1u : 0u});
}
