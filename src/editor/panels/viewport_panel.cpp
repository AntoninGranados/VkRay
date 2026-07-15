#include "viewport_panel.hpp"

#include <limits>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "imgui/imgui.h"
#include "imgui/ImGuizmo.h"

#include "core/camera.hpp"
#include "core/core.hpp"
#include "core/scene/scene.hpp"
#include "core/scene/object/object.hpp"
#include "editor/ecs/systems/camera_drawing_system.hpp"
#include "editor/editor.hpp"
#include "editor/scene/raycast.hpp"
#include "editor/scene/scene_ui.hpp"
#include "editor/ui_constants.hpp"

void ViewportPanel::draw() {
    Scene& scene = Core::getScene();
    const SceneSelection& selection = Editor::getUi().getSelection();

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::AllowAxisFlip(false);
    ImGuizmo::BeginFrame();

    ui::setNextWindowFixed(true);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Viewport", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse
    );
    hovered  = ImGui::IsWindowHovered();
    pos      = ImGui::GetWindowPos();
    size     = ImGui::GetContentRegionAvail();
    drawList = ImGui::GetWindowDrawList();
    ImGui::Image(Editor::getEditorRenderer().getDisplayTexId(), size);

    if (ImGui::IsItemHovered()) {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGuizmo::IsOver()) {
            ImVec2 mp = ImGui::GetMousePos();
            float dist;
            int entityId = raycast(scene, { mp.x - pos.x, mp.y - pos.y }, dist);
            if (onEntitySelection) onEntitySelection(entityId);
        }
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle)) {
            ImVec2 mp = ImGui::GetMousePos();
            float dist;
            int hit = raycast(scene, { mp.x - pos.x, mp.y - pos.y }, dist, false);
            if (hit >= 0) {
                scene.getCamera().setFocusDepth(dist);
                Core::requestAccumulationRestart();
            }
        }
    }

    ImGui::End();
    ImGui::PopStyleVar();

    if (!drawList) return;

    ImGuizmo::SetDrawlist(drawList);
    ImGuizmo::SetRect(pos.x, pos.y, size.x, size.y);

    ecs::cameraDrawingSystem(scene.getRegistry());
    drawGizmo(scene, selection);
}

void ViewportPanel::drawGizmo(Scene& scene, const SceneSelection& selection) {
    if (selection.entity < 0) return;

    ecs::Entity e = scene.getEntities()[static_cast<size_t>(selection.entity)];
    if (!scene.getRegistry().has<ecs::Transform>(e)) return;

    ecs::Transform& t = scene.getRegistry().get<ecs::Transform>(e);
    glm::mat4 model = t.local;

    const Camera& camera = scene.getCamera();
    const float aspect = size.y > 0.0f ? size.x / size.y : 1.0f;
    const glm::mat4 view = camera.getView();
    const glm::mat4 proj = camera.getProjection(aspect);

    int opFlags = ImGuizmo::OPERATION::TRANSLATE | ImGuizmo::OPERATION::ROTATE | ImGuizmo::OPERATION::SCALE;
    // Keep gizmo orientation in world space: avoid mixing scale with other ops.
    if ((opFlags & ImGuizmo::OPERATION::SCALE) && (opFlags & (ImGuizmo::OPERATION::TRANSLATE | ImGuizmo::OPERATION::ROTATE))) {
        opFlags &= ~ImGuizmo::OPERATION::SCALE;
    }

    ImGuizmo::PushID(selection.entity);
    if (ImGuizmo::Manipulate(
            glm::value_ptr(view),
            glm::value_ptr(proj),
            static_cast<ImGuizmo::OPERATION>(opFlags),
            ImGuizmo::MODE::WORLD,
            glm::value_ptr(model))) {
        if (isInvalid(model)) {
            ImGuizmo::PopID();
            return;
        }

        // TODO: move transform update to a better place
        glm::vec3 translation, rotationEuler, scale;
        ImGuizmo::DecomposeMatrixToComponents(
            glm::value_ptr(model),
            glm::value_ptr(translation),
            glm::value_ptr(rotationEuler),
            glm::value_ptr(scale));

        t.setPosition(translation);
        t.setRotation(glm::quat(glm::radians(rotationEuler)));
        t.setScale(scale);
        scene.update();
    }
    ImGuizmo::PopID();
}

int ViewportPanel::raycast(Scene& scene, const glm::vec2& screenPos, float& dist, bool includeCameras) {
    const Ray ray = getRay(screenPos, { size.x, size.y }, scene.getCamera());
    float tClosest = std::numeric_limits<float>::infinity();
    int idClosest = -1;

    auto& sphereStorage    = scene.getRegistry().storage<ecs::Sphere>();
    auto& planeStorage     = scene.getRegistry().storage<ecs::Plane>();
    auto& boxStorage       = scene.getRegistry().storage<ecs::Box>();
    auto& quadStorage      = scene.getRegistry().storage<ecs::Quad>();
    auto& meshStorage      = scene.getRegistry().storage<ecs::MeshRef>();
    auto& cameraStorage    = scene.getRegistry().storage<ecs::CameraObject>();
    auto& transformStorage = scene.getRegistry().storage<ecs::Transform>();

    for (size_t i = 0; i < scene.getEntities().size(); i++) {
        const ecs::Entity& e = scene.getEntities()[i];
        if (!transformStorage.has(e)) continue;

        auto& transform = transformStorage.get(e);
        float t = -1.0f;

        if (sphereStorage.has(e)) {
            t = raySphereIntersection(ray, transform.position, sphereStorage.get(e).radius);
        } else if (planeStorage.has(e)) {
            glm::vec3 normal = glm::normalize(transform.rotation * glm::vec3(0.0f, 1.0f, 0.0f));
            t = rayPlaneIntersection(ray, transform.position, normal);
        } else if (boxStorage.has(e)) {
            t = rayBoxIntersection(ray, transform.local);
        } else if (quadStorage.has(e)) {
            const ecs::Quad& q = quadStorage.get(e);
            t = rayQuadIntersection(ray, transform.position, q.u, q.v, q.normal);
        } else if (meshStorage.has(e)) {
            const ecs::MeshRef& meshRef = meshStorage.get(e);
            if (meshRef.handle >= 0 && static_cast<size_t>(meshRef.handle) < scene.getMeshAssets().size()) {
                const MeshAsset& asset = scene.getMeshAssets()[meshRef.handle];
                t = rayMeshIntersection(ray, transform.local, asset.getVertices(), asset.getIndices());
            }
        } else if (includeCameras && cameraStorage.has(e)) {
            if (cameraStorage.get(e).isPreview) continue;
            constexpr float cameraSelectRadius = 0.6f;
            t = raySphereIntersection(ray, transform.position, cameraSelectRadius);
        }

        if (t >= 0.0f && t < tClosest) {
            tClosest = t;
            idClosest = static_cast<int>(i);
        }
    }

    dist = tClosest;
    return idClosest;
}
