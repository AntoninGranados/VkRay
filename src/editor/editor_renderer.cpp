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

int resolveSelectedObjectIndex(const ecs::Registry& reg, ecs::Entity selected) {
    const std::unordered_map<ecs::Entity, int>& objectIndices = reg.ctx().get<ObjectIndices>().byEntity;
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
    if (reg.has(selected, ecs::TiltShiftLens)) {
        const ecs::Component& ts = reg.get(selected, ecs::TiltShiftLens);
        normal =
            glm::normalize(glm::quat(glm::radians(ts.get<glm::vec3>("plane_rotation"))) * glm::vec3(0.0f, 0.0f, 1.0f));
        point = ts.get<glm::vec3>("plane_position");
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
    renderExtent = Core::getRenderExtent();

    displayImageHandle =
        engine.createImage("DisplayImage", VK_FORMAT_R32G32B32A32_SFLOAT, viewportExtent.width, viewportExtent.height);
    debugImageHandle =
        engine.createImage("DebugImage", VK_FORMAT_R32G32B32A32_SFLOAT, renderExtent.width, renderExtent.height);

    debugUBOHandle = engine.createBuffer("DebugUBO", sizeof(DebugUBO));
    displayUBOHandle = engine.createBuffer("DisplayUBO", sizeof(DisplayUBO));

    registerImGuiTextures();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
}

void EditorRenderer::addPasses(VkSmol::RenderGraph& graph) {
    VkSmol::Engine& engine = Core::getEngine();
    const RenderResources& renderResources = Core::getCoreRenderer().getResources();
    const SceneGpuBuffers& sceneHandles = renderResources.sceneHandles;

    // Display pass — beauty + selection edges + focus plane overlay (compute, at viewport resolution)
    graph.addComputePass(kDisplayPassName)
        .pipeline("builtin:/shaders/editor/display.slang", {}, ScriptTable::conformances({&ScriptTable::kLens}))
        .read("ubo", displayUBOHandle)
        .read("outputTex", renderResources.outputImageHandle)
        .read("pixelInfoBuffer", renderResources.pixelInfoBufferHandle)
        .write("displayOut", displayImageHandle)
        .read("scene.vertices", sceneHandles.vertex)
        .read("scene.indices", sceneHandles.index)
        .read("scene.bvhNodes", sceneHandles.bvh)
        .read("scene.meshes", sceneHandles.mesh)
        .read("scene.materials", sceneHandles.material)
        .read("scene.pluginParams", sceneHandles.pluginParams)
        .read("scene.objects", sceneHandles.object)
        .read("scene.motion", sceneHandles.liveMotion)
        .execute([this](VkSmol::PassContext& context) {
            context.dispatchThreads(viewportExtent.width, viewportExtent.height);
        });

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
    displayUBO.camera = buildCameraUBO(reg, camera, aspect);
    displayUBO.camera.thinLens.lensRadius = 0.0f;

    displayUBO.selectedObjectId = -1;
    displayUBO.showFocusPlane = 0;
    displayUBO.previewBorderEnabled =
        (Core::getRenderMode() == RenderMode::Preview && Editor::isUsingSceneCamera()) ? 1 : 0;

    const glm::ivec2 outputRenderSize = Core::getParameters().get<glm::ivec2>("renderer/output/render_size");
    const float outputAspect = outputRenderSize.y > 0
                                   ? static_cast<float>(outputRenderSize.x) / static_cast<float>(outputRenderSize.y)
                                   : aspect;
    displayUBO.previewFrameExtent = EditorCameraPreview::frameExtent(reg, camera, aspect, outputAspect);

    if (selectedEntity.has_value()) {
        const ecs::Entity e = *selectedEntity;
        displayUBO.selectedObjectId = resolveSelectedObjectIndex(reg, e);

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
}
