#include "editor_renderer.hpp"

#include "VkSmol/engine.hpp"
#include "VkSmol/graph/builder_resource.hpp"
#include "VkSmol/graph/pass/compute_pass_builder.hpp"
#include "VkSmol/graph/pass/graphics_pass_builder.hpp"
#include "VkSmol/graph/pass/present_pass_builder.hpp"
#include "VkSmol/graph/render_graph_builder.hpp"

#include <glm/gtc/quaternion.hpp>

#include "core/camera/camera.hpp"
#include "core/ecs/components/component.hpp"
#include "core/ecs/components/core.hpp"
#include "core/ecs/entity.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_vulkan.h"

#include "core/core.hpp"
#include "core/parameters/parameters.hpp"
#include "editor.hpp"
#include "editor/ui_utils.hpp"

namespace {

int resolveSelectedObjectIndex(const ecs::Registry& reg, ecs::Entity selected) {
    const auto& allTransforms = reg.storage(ecs::Transform);
    int flatIdx = -1;
    int i = 0;
    auto check = [&](const ecs::ComponentType& type) {
        if (flatIdx >= 0) return;
        for (const auto& ent : reg.storage(type).entities()) {
            if (!allTransforms.has(ent)) continue;
            if (ent == selected) { flatIdx = i; return; }
            i++;
        }
    };
    check(ecs::Sphere);
    check(ecs::Plane);
    check(ecs::Box);
    check(ecs::Quad);
    check(ecs::MeshRef);
    return flatIdx;
}

struct FocusPlane {
    bool visible = false;
    glm::vec4 plane{};
};

FocusPlane resolveFocusPlane(const ecs::Registry& reg, ecs::Entity selected, const ecs::Entity& camera) {
    if (!reg.has(selected, ecs::ThinLens) || !reg.get(selected, ecs::ThinLens).get<bool>("show_focus_plane"))
        return {};

    const ecs::Component& t = reg.get(camera, ecs::Transform);

    glm::vec3 normal, point;
    if (reg.has(selected, ecs::TiltShiftLens)) {
        const ecs::Component& ts = reg.get(selected, ecs::TiltShiftLens);
        normal = glm::normalize(
            glm::quat(glm::radians(ts.get<glm::vec3>("plane_rotation"))) * glm::vec3(0.0f, 0.0f, 1.0f));
        point = ts.get<glm::vec3>("plane_position");
    } else {
        normal = directionFromRotation(t.get<glm::vec3>("rotation"));
        point  = t.get<glm::vec3>("position") + normal * reg.get(selected, ecs::ThinLens).get<float>("focal_distance");
    }
    float d = -glm::dot(normal, point);
    if (glm::dot(t.get<glm::vec3>("position"), normal) + d > 0.0f) { normal = -normal; d = -d; }
    return { true, glm::vec4(normal, d) };
}

} // namespace

void EditorRenderer::initGraph(RenderGraphBuilder& builder, RenderResources& renderResources) {
    VkSmol& engine = Core::getEngine();
    editorGroupHandle = builder.addSubmissionGroup("Editor");
    uiGroupHandle     = builder.addSubmissionGroup("Ui");

    swapchainImageHandle = builder.createImage(
        "SwapchainImage",
        VK_FORMAT_R32G32B32A32_SFLOAT,
        engine.getExtent().width, engine.getExtent().height, 1,
        VKSMOL_IMAGE_OWNERSHIP_IMPORTED,
        0,
        ImageAccessInfo{ .usage = ImageUsageType::Undefined, .access = AccessType::None },
        ImageAccessInfo{ .usage = ImageUsageType::Present,   .access = AccessType::Read }
    );

    displayImageHandle = builder.createImage(
        "DisplayImage",
        VK_FORMAT_R32G32B32A32_SFLOAT,
        engine.getExtent().width, engine.getExtent().height
    );
    debugImageHandle = builder.createImage(
        "DebugImage",
        VK_FORMAT_R32G32B32A32_SFLOAT,
        engine.getExtent().width, engine.getExtent().height
    );
    outputImageHandle = renderResources.outputImageHandle;

    debugUBOHandle   = builder.createBuffer("DebugUBO",   sizeof(DebugUBO),   VKSMOL_BUFFER_CREATE_PER_FRAME_BIT, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    displayUBOHandle = builder.createBuffer("DisplayUBO", sizeof(DisplayUBO), VKSMOL_BUFFER_CREATE_PER_FRAME_BIT, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    // Display pass — beauty + selection edges + focus plane overlay (compute, at viewport resolution)
    ComputePassBuilder display = builder.addComputePass("DisplayPass");
    displayPassHandle = display.getHandle();
    display.setGroup(editorGroupHandle);
    display.readImage ( 0, renderResources.outputImageHandle,              ImageUsageType::Sampled);
    display.readBuffer( 1, renderResources.pixelInfoBufferHandle,          BufferUsageType::Storage);
    display.writeImage( 2, displayImageHandle,                           ImageUsageType::Storage);
    display.readBuffer( 3, displayUBOHandle,                             BufferUsageType::Uniform);
    display.readBuffer( 4, renderResources.sceneHandles.sphere.handle,     BufferUsageType::Storage);
    display.readBuffer( 5, renderResources.sceneHandles.plane.handle,      BufferUsageType::Storage);
    display.readBuffer( 6, renderResources.sceneHandles.box.handle,        BufferUsageType::Storage);
    display.readBuffer( 7, renderResources.sceneHandles.quad.handle,       BufferUsageType::Storage);
    display.readBuffer( 8, renderResources.sceneHandles.vertex.handle,     BufferUsageType::Storage);
    display.readBuffer( 9, renderResources.sceneHandles.index.handle,      BufferUsageType::Storage);
    display.readBuffer(10, renderResources.sceneHandles.bvh.handle,        BufferUsageType::Storage);
    display.readBuffer(11, renderResources.sceneHandles.mesh.handle,       BufferUsageType::Storage);
    display.readBuffer(12, renderResources.sceneHandles.object.handle,     BufferUsageType::Storage);
    display.readBuffer(13, renderResources.sceneHandles.material.handle,   BufferUsageType::Storage);
    display.setPipeline("./src/shaders/editor/display.glsl");
    displayTimestamp = display.setTimestamp();

    // Debug pass — debug view visualization (compute)
    ComputePassBuilder debug = builder.addComputePass("DebugPass");
    debugPassHandle = debug.getHandle();
    debug.setGroup(editorGroupHandle);
    debug.readBuffer( 0, debugUBOHandle, BufferUsageType::Uniform);
    debug.readBuffer( 1, renderResources.pixelInfoBufferHandle, BufferUsageType::Storage);
    debug.writeImage( 2, debugImageHandle, ImageUsageType::Storage);
    debug.setPipeline("./src/shaders/editor/debug.glsl");
    debugTimestamp = debug.setTimestamp();

    // UI pass — ImGui renders over cleared swapchain; declares sampled reads to drive barriers
    GraphicsPassBuilder ui = builder.addGraphicsPass("UiPass");
    uiPassHandle = ui.getHandle();
    ui.setGroup(uiGroupHandle);
    ui.readImage(0, displayImageHandle, ImageUsageType::Sampled);
    ui.readImage(1, debugImageHandle,   ImageUsageType::Sampled);
    ui.readImage(2, renderResources.outputImageHandle, ImageUsageType::Sampled);
    ui.writeImage(swapchainImageHandle, ImageUsageType::ColorAttachment, WriteMode::Overwrite, AttachmentLoad::Clear);
    uiTimestamp = ui.setTimestamp();

    PresentPassBuilder present = builder.addPresentPass("PresentPass");
    presentPassHandle = present.getHandle();
    present.setGroup(uiGroupHandle);
    present.setPresentationImage(swapchainImageHandle);

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
}

void EditorRenderer::registerImGuiTextures() {
    VkSmol& engine = Core::getEngine();

    if (outputTexId)  ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet)outputTexId);
    if (displayTexId) ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet)displayTexId);
    if (debugTexId)   ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet)debugTexId);

    outputTexId = (ImTextureID)ImGui_ImplVulkan_AddTexture(
        engine.getSampler(outputImageHandle).get(),
        engine.getView(outputImageHandle).get(),
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );
    displayTexId = (ImTextureID)ImGui_ImplVulkan_AddTexture(
        engine.getSampler(displayImageHandle).get(),
        engine.getView(displayImageHandle).get(),
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );
    debugTexId = (ImTextureID)ImGui_ImplVulkan_AddTexture(
        engine.getSampler(debugImageHandle).get(),
        engine.getView(debugImageHandle).get(),
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );
}

void EditorRenderer::resize(VkExtent2D renderExt, VkExtent2D viewportExt) {
    VkSmol& engine = Core::getEngine();
    renderExtent   = renderExt;
    viewportExtent = viewportExt;
    engine.resizeImage(displayImageHandle, viewportExt.width, viewportExt.height);
    engine.resizeImage(debugImageHandle,   renderExt.width,   renderExt.height);
    registerImGuiTextures();
}

void EditorRenderer::render(const FrameContext& frameContext) {
    VkSmol& engine = Core::getEngine();
    const VkExtent2D renderE   = renderExtent.width   > 0 ? renderExtent   : frameContext.extent;
    const VkExtent2D viewportE = viewportExtent.width > 0 ? viewportExtent : frameContext.extent;

    debugUBO.debugView = Core::getParameters().get<DebugView>("renderer/debug_view");

    Scene& scene = Core::getScene();
    const ecs::Entity& camera = scene.getCamera();
    const std::optional<ecs::Entity> selectedEntity = Editor::getSelectedEntity();
    const ecs::Registry& reg = scene.getRegistry();
    {
        const ecs::Component& t = reg.get(camera, ecs::Transform);
        const glm::vec3 dir   = directionFromRotation(t.get<glm::vec3>("rotation"));
        const glm::vec3 right = glm::normalize(glm::cross(dir, glm::vec3(0.0f, 1.0f, 0.0f)));
        const glm::vec3 camUp = glm::cross(right, dir);
        const float tanHFov   = glm::tan(glm::radians(effectiveFov(reg, camera)) * 0.5f);
        const float aspect    = viewportE.height > 0
            ? static_cast<float>(viewportE.width) / static_cast<float>(viewportE.height) : 1.0f;
        displayUBO.camera.eye = t.get<glm::vec3>("position");
        displayUBO.camera.U   = right * aspect * tanHFov;
        displayUBO.camera.V   = camUp * tanHFov;
        displayUBO.camera.W   = dir;
    }

    displayUBO.selectedObjectId = -1;
    displayUBO.showFocusPlane = 0;
    displayUBO.previewBorderEnabled = (Core::getRenderMode() == RenderMode::Preview && scene.isPreviewing()) ? 1 : 0;

    if (selectedEntity.has_value()) {
        const ecs::Entity e = *selectedEntity;
        displayUBO.selectedObjectId = resolveSelectedObjectIndex(reg, e);

        if (scene.isPreviewing()) {
            const FocusPlane focus = resolveFocusPlane(reg, e, camera);
            if (focus.visible) {
                displayUBO.showFocusPlane = 1;
                displayUBO.focusPlane     = focus.plane;
            }
        }
    }

    engine.fillBuffer(engine.getBuffer(debugUBOHandle,   frameContext.currentFrame), &debugUBO);
    engine.fillBuffer(engine.getBuffer(displayUBOHandle, frameContext.currentFrame), &displayUBO);

    engine.bindImage(
        swapchainImageHandle,
        engine.getSwapchainImage(frameContext.imageIndex).get(),
        engine.getSwapchainImageView(frameContext.imageIndex).get()
    );

    {
        CommandBuffer& cmd = engine.beginRecording(editorGroupHandle);

        engine.dispatch(cmd, displayPassHandle, (viewportE.width + 7) / 8, (viewportE.height + 7) / 8);
        engine.dispatch(cmd, debugPassHandle,   (renderE.width   + 7) / 8, (renderE.height   + 7) / 8);
     
        engine.endRecording(editorGroupHandle);
    }
    
    {
        CommandBuffer& commandBuffer = engine.beginRecording(uiGroupHandle);
     
        engine.beginGraphics(commandBuffer, uiPassHandle, &ui::kDraculaBg.x);
        Editor::getUi().draw(commandBuffer);
        engine.endGraphics(commandBuffer, uiPassHandle);
    
        engine.emitBarriers(commandBuffer, presentPassHandle);
    
        // @warning this is out of place, but I need a command buffer
        engine.emitOutputBarriers(commandBuffer);
    
        engine.endRecording(uiGroupHandle);
    }
}
