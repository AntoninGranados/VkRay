#include "viewport_panel.hpp"

#include <limits>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "core/ecs/components/camera.hpp"
#include "core/ecs/entity.hpp"
#include "imgui/imgui.h"
#include "imgui/ImGuizmo.h"

#include "core/camera/camera.hpp"
#include "core/core.hpp"
#include "core/scene/scene.hpp"
#include "core/scene/gpu_structs.hpp"
#include "editor/ecs/systems/camera_drawing_system.hpp"
#include "editor/editor.hpp"
#include "editor/scene/raycast.hpp"
#include "editor/ui_utils.hpp"

void ViewportPanel::draw() {
    Scene& scene = Core::getScene();

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::AllowAxisFlip(false);
    ImGuizmo::BeginFrame();

    ui::setNextWindowFixed(true);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    drawList = nullptr;
    ui::drawWindow(getTitle(),
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse,
        [&] {
            hovered = ImGui::IsWindowHovered();
            pos = ImGui::GetWindowPos();
            size = ImGui::GetContentRegionAvail();
            drawList = ImGui::GetWindowDrawList();
            ImGui::Image(Editor::getEditorRenderer().getDisplayTexId(), size);

            if (ImGui::IsItemHovered()) {
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGuizmo::IsOver()) {
                    ImVec2 mp = ImGui::GetMousePos();
                    float dist;
                    auto entityId = raycast(scene, { mp.x - pos.x, mp.y - pos.y }, dist);
                    if (onEntitySelection) onEntitySelection(entityId);
                }
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle) && scene.getRegistry().has(scene.getCamera(), ecs::ThinLens)) {
                    ImVec2 mp = ImGui::GetMousePos();
                    float dist;
                    auto hit = raycast(scene, { mp.x - pos.x, mp.y - pos.y }, dist, false);
                    if (hit.has_value()) {
                        scene.getRegistry().get(scene.getCamera(), ecs::ThinLens).set<float>("focus_distance", dist);
                        Core::markRenderDirty();
                    }
                }
            }
        }
    );
    ImGui::PopStyleVar();

    if (!drawList) return;

    ImGuizmo::SetDrawlist(drawList);
    ImGuizmo::SetRect(pos.x, pos.y, size.x, size.y);

    ecs::cameraDrawingSystem(scene.getRegistry());
    drawGizmo(scene);
}

void ViewportPanel::drawGizmo(Scene& scene) {
    const std::optional<ecs::Entity> selectedEntity = Editor::getSelectedEntity();
    if (!selectedEntity.has_value()) return;

    const ecs::Entity e = *selectedEntity;
    if (!scene.getRegistry().has(e, ecs::Transform)) return;

    ecs::Component& t = scene.getRegistry().get(e, ecs::Transform);
    glm::mat4 model = glm::translate(glm::mat4(1.0f), t.get<glm::vec3>("position"))
        * glm::mat4_cast(glm::quat(glm::radians(t.get<glm::vec3>("rotation"))))
        * glm::scale(glm::mat4(1.0f), t.get<glm::vec3>("scale"));

    const ecs::Entity& camera = scene.getCamera();
    const float aspect = size.y > 0.0f ? size.x / size.y : 1.0f;
    const glm::mat4 view = getView(scene.getRegistry(), camera);
    const glm::mat4 proj = getProjection(scene.getRegistry(), camera, aspect);

    // Keep gizmo orientation in world space: avoid mixing scale with other ops.
    const int opFlags = ImGuizmo::OPERATION::TRANSLATE | ImGuizmo::OPERATION::ROTATE;

    ImGuizmo::PushID(static_cast<int>(e.getId()) * 2);
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
        glm::vec3 translation, scale, skew;
        glm::quat rotation;
        glm::vec4 perspective;
        glm::decompose(model,
            scale, rotation, translation,
            skew, perspective
        );

        const glm::quat oldRotation = glm::quat(glm::radians(t.get<glm::vec3>("rotation")));

        t.set<glm::vec3>("position", translation);
        t.set<glm::vec3>("scale", scale);
        if (glm::abs(glm::dot(oldRotation, rotation)) < 0.99999f)
            t.set<glm::vec3>("rotation", glm::degrees(glm::eulerAngles(rotation)));
        Core::markRenderDirty();
    }
    ImGuizmo::PopID();

    if (!scene.getRegistry().has(e, ecs::TiltShiftLens)) return;
    ecs::Component& ts = scene.getRegistry().get(e, ecs::TiltShiftLens);

    glm::mat4 tsModel = glm::translate(glm::mat4(1.0f), ts.get<glm::vec3>("plane_position"))
        * glm::mat4_cast(glm::quat(glm::radians(ts.get<glm::vec3>("plane_rotation"))));

    ImGuizmo::PushID(static_cast<int>(e.getId()) * 2 + 1);
    if (ImGuizmo::Manipulate(
            glm::value_ptr(view),
            glm::value_ptr(proj),
            static_cast<ImGuizmo::OPERATION>(ImGuizmo::OPERATION::TRANSLATE | ImGuizmo::OPERATION::ROTATE),
            ImGuizmo::MODE::WORLD,
            glm::value_ptr(tsModel))) {
        if (!isInvalid(tsModel)) {
            glm::vec3 translation, rotationEuler, scale;
            ImGuizmo::DecomposeMatrixToComponents(
                glm::value_ptr(tsModel),
                glm::value_ptr(translation),
                glm::value_ptr(rotationEuler),
                glm::value_ptr(scale));
            ts.set<glm::vec3>("plane_position", translation);
            ts.set<glm::vec3>("plane_rotation", rotationEuler);
            Core::markRenderDirty();
        }
    }
    ImGuizmo::PopID();
}

std::optional<ecs::Entity> ViewportPanel::raycast(Scene& scene, const glm::vec2& screenPos, float& dist, bool includeCameras) {
    const Ray ray = getRay(screenPos, { size.x, size.y }, scene.getCamera());
    float tClosest = std::numeric_limits<float>::infinity();
    std::optional<ecs::Entity> entityClosest;

    auto& planeStorage = scene.getRegistry().storage(ecs::Plane);
    auto& boxStorage = scene.getRegistry().storage(ecs::Box);
    auto& quadStorage = scene.getRegistry().storage(ecs::Quad);
    auto& meshRefs = scene.getRegistry().storage(ecs::MeshRef);
    auto& cameraStorage = scene.getRegistry().storage(ecs::Camera);
    auto& transformStorage = scene.getRegistry().storage(ecs::Transform);

    const ecs::Entity& activeCamera = scene.getCamera();
    for (const ecs::Entity& e : scene.getChildren(scene.getObjectsRoot())) {
        if (!transformStorage.has(e)) continue;

        const ecs::Component& transform = transformStorage.get(e);
        const glm::vec3 tPos = transform.get<glm::vec3>("position");
        const glm::quat tRot = glm::quat(glm::radians(transform.get<glm::vec3>("rotation")));
        float t = -1.0f;

        if (scene.getRegistry().has(e, ecs::Sphere)) {
            t = raySphereIntersection(ray, tPos, scene.getRegistry().get(e, ecs::Sphere).get<float>("radius"));
        } else if (planeStorage.has(e)) {
            t = rayPlaneIntersection(ray, tPos, glm::normalize(tRot * glm::vec3(0.0f, 1.0f, 0.0f)));
        } else if (boxStorage.has(e)) {
            const glm::mat4 local = glm::translate(glm::mat4(1.0f), tPos)
                * glm::mat4_cast(tRot)
                * glm::scale(glm::mat4(1.0f), transform.get<glm::vec3>("scale"));
            t = rayBoxIntersection(ray, local);
        } else if (quadStorage.has(e)) {
            const glm::vec3 scale = transform.get<glm::vec3>("scale");
            const glm::vec3 u = tRot * glm::vec3(1.0f, 0.0f, 0.0f) * scale.x;
            const glm::vec3 v = tRot * glm::vec3(0.0f, 1.0f, 0.0f) * scale.y;
            const glm::vec3 normal = tRot * glm::vec3(0.0f, 0.0f, 1.0f);
            t = rayQuadIntersection(ray, tPos - 0.5f * (u + v), u, v, normal);
        } else if (meshRefs.has(e)) {
            const ecs::Entity meshEntity = meshRefs.get(e).get<ecs::Entity>("handle");
            if (const MeshAsset* asset = scene.getMeshAsset(meshEntity)) {
                const glm::mat4 local = glm::translate(glm::mat4(1.0f), tPos)
                    * glm::mat4_cast(tRot)
                    * glm::scale(glm::mat4(1.0f), transform.get<glm::vec3>("scale"));
                t = rayMeshIntersection(ray, local, asset->getVertices(), asset->getIndices());
            }
        } else if (includeCameras && cameraStorage.has(e)) {
            if (activeCamera == e) continue;
            constexpr float cameraSelectRadius = 0.6f;
            t = raySphereIntersection(ray, tPos, cameraSelectRadius);
        }

        if (t >= 0.0f && t < tClosest) {
            tClosest = t;
            entityClosest = e;
        }
    }

    dist = tClosest;
    return entityClosest;
}
