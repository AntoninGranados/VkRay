#include "editor_renderer.hpp"

#include <unordered_map>

#include "VkSmol/engine.hpp"
#include "VkSmol/graph/pass/compute_pass_builder.hpp"
#include "VkSmol/graph/pass/graphics_pass_builder.hpp"
#include "VkSmol/graph/pass_context.hpp"

#include <glm/gtc/quaternion.hpp>

#include "imgui/imgui.h"

#include "core/camera/camera.hpp"
#include "core/core.hpp"
#include "core/ecs/components/component.hpp"
#include "core/ecs/components/core.hpp"
#include "core/ecs/entity.hpp"
#include "core/fields/parameters.hpp"
#include "core/render/script_table.hpp"
#include "core/scene/scene.hpp"
#include "editor.hpp"
#include "editor/camera_preview.hpp"
#include "editor/ui_utils.hpp"

namespace {

int resolveSelectedObjectIndex(const Scene& scene, ecs::Entity selected) {
    const std::unordered_map<ecs::Entity, int>& objectIndices = scene.getPackingInfo().objectIndices;
    const auto it = objectIndices.find(selected);
    return it != objectIndices.end() ? it->second : -1;
}

struct FocusPlane {
    bool visible = false;
    glm::vec4 plane{};
};

FocusPlane resolveFocusPlane(const ecs::Registry& reg, ecs::Entity selected, const ecs::Entity& camera) {
    if (!reg.has(selected, ecs::Camera) || !reg.get(selected, ecs::Camera).get<bool>("show_focus_plane")) return {};

    const ecs::Component& t = reg.get(camera, ecs::Transform);

    glm::vec3 normal, point;
    if (const ecs::Component* ts = ScriptTable::tiltShift(reg, selected)) {
        normal =
            glm::normalize(glm::quat(glm::radians(ts->get<glm::vec3>("planeRotation"))) * glm::vec3(0.0f, 0.0f, 1.0f));
        point = ts->get<glm::vec3>("planePosition");
    } else {
        normal = directionFromRotation(t.get<glm::vec3>("rotation"));
        point = t.get<glm::vec3>("position") + normal * resolveFocusDistance(reg, selected);
    }
    float d = -glm::dot(normal, point);
    if (glm::dot(t.get<glm::vec3>("position"), normal) + d > 0.0f) {
        normal = -normal;
        d = -d;
    }
    return {true, glm::vec4(normal, d)};
}

} // namespace

void EditorRenderer::init() {
    VkSmol::Engine& engine = Core::getEngine();
    viewportExtent = engine.getExtent();
    renderExtent = Core::getCoreRenderer().getRenderExtent();

    displayImageHandle =
        engine.createImage("DisplayImage", VK_FORMAT_R32G32B32A32_SFLOAT, viewportExtent.width, viewportExtent.height);
    rasterImageHandle =
        engine.createImage("RasterImage", VK_FORMAT_R32G32B32A32_SFLOAT, viewportExtent.width, viewportExtent.height);
    depthImageHandle =
        engine.createImage("DepthImage", VK_FORMAT_D32_SFLOAT, viewportExtent.width, viewportExtent.height);
    debugImageHandle =
        engine.createImage("DebugImage", VK_FORMAT_R32G32B32A32_SFLOAT, renderExtent.width, renderExtent.height);

    debugUBOHandle = engine.createBuffer("DebugUBO", sizeof(DebugUBO));
    displayUBOHandle = engine.createBuffer("DisplayUBO", sizeof(DisplayUBO));
    overlayDrawer.init();
    rasterRenderer.init();

    registerImGuiTextures();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
}

void EditorRenderer::addPasses(VkSmol::RenderGraph& graph) {
    VkSmol::Engine& engine = Core::getEngine();
    const RenderResources& renderResources = Core::getCoreRenderer().getResources();
    const SceneGpuBuffers& sceneHandles = renderResources.sceneHandles;
    shading = Core::getParameters().get<ViewportShading>("renderer/viewport/shading");
    const bool rasterized = shading == ViewportShading::Rasterized;

    rasterRenderer.addPasses(graph, sceneHandles, rasterImageHandle, depthImageHandle, rasterized);

    // Display pass — beauty + selection edges + focus plane overlay (fullscreen graphics, at viewport resolution)
    graph.addGraphicsPass(kDisplayPassName)
        .pipeline("builtin:/shaders/editor/display.slang", "builtin:/shaders/editor/display.slang", {},
                  {.primitive = {.cullMode = VK_CULL_MODE_NONE},
                   .depth = std::nullopt,
                   .blend = {VkSmol::BlendState::opaque()}},
                  ScriptTable::conformances({&ScriptTable::kLens}))
        .read("ubo", displayUBOHandle)
        .read("outputTex", rasterized ? rasterImageHandle : renderResources.outputImageHandle)
        .read("depthTex", depthImageHandle)
        .colorAttachment(displayImageHandle, VkSmol::AttachmentLoad::DontCare)
        .read("scene.vertices", sceneHandles.vertex)
        .read("scene.indices", sceneHandles.index)
        .read("scene.bvhNodes", sceneHandles.bvh)
        .read("scene.meshes", sceneHandles.mesh)
        .read("scene.materials", sceneHandles.material)
        .read("scene.pluginParams", sceneHandles.pluginParams)
        .read("scene.objects", sceneHandles.object)
        .read("scene.motion", sceneHandles.liveMotion)
        .read("scene.textures", sceneHandles.textures)
        .execute([](VkSmol::PassContext& context) { context.draw(3); });

    overlayDrawer.addPasses(graph, displayImageHandle, depthImageHandle);

    // Debug pass — debug view visualization (compute)
    graph.addComputePass(kDebugPassName)
        .pipeline("builtin:/shaders/editor/debug.slang")
        .read("ubo", debugUBOHandle)
        .read("pixelInfoBuffer", renderResources.pixelInfoBufferHandle)
        .write("debugOut", debugImageHandle)
        .execute(
            [this](VkSmol::PassContext& context) { context.dispatchThreads(renderExtent.width, renderExtent.height); });

    // UI pass — ImGui renders over cleared swapchain; declares sampled reads to drive barriers
    const MaterialPreview& materialPreview = Editor::getMaterialPreview();
    graph.addGraphicsPass(kUiPassName)
        .read(displayImageHandle, VkSmol::ImageUsageType::Sampled)
        .read(debugImageHandle, VkSmol::ImageUsageType::Sampled)
        .read(renderResources.outputImageHandle, VkSmol::ImageUsageType::Sampled)
        .read(materialPreview.getOutputImageHandle(), VkSmol::ImageUsageType::Sampled)
        .read(materialPreview.getThumbnailImageHandle(), VkSmol::ImageUsageType::Sampled)
        .colorAttachment(engine.getSwapchainImage(), VkSmol::AttachmentLoad::Clear,
                         {ui::kDraculaBg.x, ui::kDraculaBg.y, ui::kDraculaBg.z, ui::kDraculaBg.w})
        .execute([](VkSmol::PassContext& context) {
            Editor::getUi().draw();
            Editor::getImGui().render(context);
        });

    graph.present(engine.getSwapchainImage());
}

void EditorRenderer::registerImGuiTextures() {
    VkSmol::Engine& engine = Core::getEngine();
    const VkSmol::ImageHandle outputImageHandle = Core::getCoreRenderer().getOutputImageHandle();

    outputTex = ui::ImGuiTexture(engine.getSampler(outputImageHandle), engine.getImageView(outputImageHandle),
                                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    displayTex = ui::ImGuiTexture(engine.getSampler(displayImageHandle), engine.getImageView(displayImageHandle),
                                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    debugTex = ui::ImGuiTexture(engine.getSampler(debugImageHandle), engine.getImageView(debugImageHandle),
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void EditorRenderer::resize(VkExtent2D renderExt, VkExtent2D viewportExt) {
    VkSmol::Engine& engine = Core::getEngine();
    renderExtent = renderExt;
    viewportExtent = viewportExt;
    engine.resizeImage(displayImageHandle, viewportExt.width, viewportExt.height);
    engine.resizeImage(rasterImageHandle, viewportExt.width, viewportExt.height);
    engine.resizeImage(depthImageHandle, viewportExt.width, viewportExt.height);
    engine.resizeImage(debugImageHandle, renderExt.width, renderExt.height);
    registerImGuiTextures();
}

void EditorRenderer::prepare() {
    VkSmol::Engine& engine = Core::getEngine();

    debugUBO.debugView = Core::getParameters().get<DebugView>("renderer/debug_view");

    Scene& scene = Core::getScene();
    const ecs::Entity camera = *scene.getActiveCamera();
    const std::optional<ecs::Entity> selectedEntity = Editor::getSelectedEntity();
    ecs::Registry& reg = scene.getRegistry();
    const float aspect = viewportExtent.height > 0
                             ? static_cast<float>(viewportExtent.width) / static_cast<float>(viewportExtent.height)
                             : 1.0f;
    const float frustumScale = Core::getCoreRenderer().getFrustumScale();
    displayUBO.camera = buildCameraUBO(scene, camera, aspect, frustumScale);
    displayUBO.camera.thinLens.lensRadius = 0.0f;

    glm::mat4 projection = getProjection(reg, camera, aspect, frustumScale);
    projection[1][1] = -projection[1][1];
    const glm::mat4 viewProjection = projection * getView(reg, camera);
    displayUBO.inverseViewProjection = glm::inverse(viewProjection);
    displayUBO.viewportSize = glm::ivec2(viewportExtent.width, viewportExtent.height);
    displayUBO.lineWidth = kOverlayLineWidth * ImGui::GetIO().DisplayFramebufferScale.x;

    displayUBO.selectedObjectId = -1;
    displayUBO.showFocusPlane = 0;
    displayUBO.previewBorderEnabled =
        (Core::getRenderMode() == RenderMode::Preview && Editor::isUsingSceneCamera()) ? 1 : 0;

    const glm::ivec2 outputRenderSize = Core::getParameters().get<glm::ivec2>("renderer/output/render_size");
    const float outputAspect = outputRenderSize.y > 0
                                   ? static_cast<float>(outputRenderSize.x) / static_cast<float>(outputRenderSize.y)
                                   : aspect;
    displayUBO.previewFrameExtent = EditorCameraPreview::frameExtent(reg, camera, aspect, outputAspect, frustumScale);

    if (selectedEntity.has_value()) {
        const ecs::Entity e = *selectedEntity;
        displayUBO.selectedObjectId = resolveSelectedObjectIndex(scene, e);

        if (Editor::isUsingSceneCamera()) {
            const FocusPlane focus = resolveFocusPlane(reg, e, camera);
            if (focus.visible) {
                displayUBO.showFocusPlane = 1;
                displayUBO.focusPlane = focus.plane;
            }
        }
    }

    engine.writeBuffer(debugUBOHandle, debugUBO);
    engine.writeBuffer(displayUBOHandle, displayUBO);
    overlayDrawer.upload(viewProjection, viewportExtent, displayUBO.lineWidth);
    rasterRenderer.prepare(scene, viewProjection, reg.get(camera, ecs::Transform).get<glm::vec3>("position"));
}
