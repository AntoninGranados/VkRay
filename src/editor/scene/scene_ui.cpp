#include "scene_ui.hpp"

#include <algorithm>
#include <format>
#include <limits>
#include <vector>

#include "FontAwesome/IconsFontAwesome7.h"
#include "imgui/imgui.h"
#include "imgui/ImGuizmo.h"

#include "utils/log.hpp"
#include "core/core.hpp"
#include "core/scene/scene.hpp"
#include "editor/ecs/component_ui_registry.hpp"
#include "editor/ui_constants.hpp"
#include "raycast.hpp"
#include "material_ui.hpp"

void SceneUI::drawGuizmo(Scene& scene, SceneSelection& selection, const glm::mat4& view, const glm::mat4& proj) {
    if (selection.entity < 0) return;

    ecs::Entity e = scene.getEntities()[static_cast<size_t>(selection.entity)];
    if (!scene.getRegistry().has<ecs::Transform>(e)) return;

    ecs::Transform& t = scene.getRegistry().get<ecs::Transform>(e);
    glm::mat4 model = t.local;

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

void SceneUI::drawInspectors(Scene& scene, SceneSelection& selection) {
    drawSelectedEntityUI(scene, selection);
    drawSelectedMaterialUI(scene, selection);
    drawSelectedMeshAssetUI(scene, selection);
}

void SceneUI::drawSelectedEntityUI(Scene& scene, SceneSelection& selection) {
    if (selection.entity < 0) return;
    ecs::Entity& e = scene.getEntities()[static_cast<size_t>(selection.entity)];
    bool openNewComponentPopup = false;

    bool open = true;
    ImGui::SetNextWindowBgAlpha(ui::kWindowBgAlpha);
    ImGui::SetNextWindowSizeConstraints({250.0f, 0.0f}, {250.0f, 600.0f});
    ImGui::Begin(
        "Entity",
        &open,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing);
    {
        ImGui::Text("Add Component");
        ImGui::SameLine();
        if (ImGui::Button("+##AddComponent", {32, 0})) {
            openNewComponentPopup = true;
        }

        auto& uiReg = ecs::ComponentUiRegistry::get();
        if (uiReg.draw(scene.getRegistry(), e)) scene.update();
    }
    ImGui::End();

    if (openNewComponentPopup) ImGui::OpenPopup("Add Component");
    ImGuiViewport* mainViewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(mainViewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Add Component", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        const auto& funcsMap = componentFuncs();
        const auto& restrictionsMap = componentRestrictions();

        std::vector<ComponentId> sortedIds;
        sortedIds.reserve(funcsMap.size());
        for (const auto& [id, _] : funcsMap) {
            sortedIds.push_back(id);
        }

        std::sort(sortedIds.begin(), sortedIds.end(), [](ComponentId a, ComponentId b) {
            const ComponentGroup groupA = componentGroup(a);
            const ComponentGroup groupB = componentGroup(b);
            if (groupA != groupB) {
                return groupA < groupB;
            }
            return componentLabel(a) < componentLabel(b);
        });

        bool firstGroup = true;
        ComponentGroup currentGroup = ComponentGroup::Other;
        for (ComponentId id : sortedIds) {
            const ComponentGroup group = componentGroup(id);
            if (firstGroup || group != currentGroup) {
                if (!firstGroup) {
                    ImGui::Spacing();
                }
                ImGui::SeparatorText(componentGroupLabel(group).c_str());
                currentGroup = group;
                firstGroup = false;
            }

            const auto& funcs = funcsMap.at(id);
            if (ImGui::Button(componentLabel(id).c_str(), ui::kButtonSize)) {
                bool verifyRestrictions = true;
                const auto& restrictions = restrictionsMap.at(id);
                for (auto& requirement : restrictions.requirements) {
                    if (!funcsMap.at(requirement).has(scene.getRegistry(), e)) {
                        verifyRestrictions = false;
                        Log::warn("SceneEditor", std::format("Missing component {}", componentLabel(requirement)));
                    }
                }
                for (auto& conflict : restrictions.conflicts) {
                    if (funcsMap.at(conflict).has(scene.getRegistry(), e)) {
                        verifyRestrictions = false;
                        Log::warn("SceneEditor", std::format("Conflicting component {}", componentLabel(conflict)));
                    }
                }

                if (verifyRestrictions) {
                    funcs.add(scene.getRegistry(), e);
                    Core::restartAccumulation();
                }
                ImGui::CloseCurrentPopup();
            }
        }

        ui::PushCancelStyleColor();
        if (ImGui::Button(ICON_FA_BAN " Cancel", ui::kButtonSize)) {
            ImGui::CloseCurrentPopup();
        }
        ui::PopCancelStyleColor();
        ImGui::EndPopup();
    }

    if (!open) selection.entity = -1;
}

void SceneUI::drawSelectedMaterialUI(Scene& scene, SceneSelection& selection) {
    if (selection.material < 0) return;

    bool open = true;
    ImGui::SetNextWindowBgAlpha(ui::kWindowBgAlpha);
    ImGui::SetNextWindowSizeConstraints({250.0f, 0.0f}, {250.0f, 600.0f});
    ImGui::Begin("Material", &open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing);
    {
        auto& mat = scene.getMaterials()[static_cast<size_t>(selection.material)];
        if (drawMaterialUI(mat)) scene.update();
    }
    ImGui::End();

    if (!open) selection.material = -1;
}

void SceneUI::drawSelectedMeshAssetUI(Scene& scene, SceneSelection& selection) {
    if (selection.meshAsset < 0) return;

    bool open = true;
    ImGui::SetNextWindowBgAlpha(ui::kWindowBgAlpha);
    ImGui::SetNextWindowSizeConstraints({250.0f, 0.0f}, {250.0f, 600.0f});
    ImGui::Begin("Mesh Asset", &open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing);
    {
        if (drawMeshAssetUI(scene.getMeshAssets()[static_cast<size_t>(selection.meshAsset)])) scene.update();
    }
    ImGui::End();

    if (!open) selection.meshAsset = -1;
}

int SceneUI::raycast(Scene& scene, const glm::vec2& screenPos, const glm::vec2& screenSize, float& dist, bool includeCameras) {
    const Ray ray = getRay(screenPos, screenSize, scene.getCamera());
    float tClosest = std::numeric_limits<float>::infinity();
    int idClosest = -1;
    float t = -1.0f;

    auto& sphereStorage = scene.getRegistry().storage<ecs::Sphere>();
    auto& planeStorage  = scene.getRegistry().storage<ecs::Plane>();
    auto& boxStorage = scene.getRegistry().storage<ecs::Box>();
    auto& quadStorage = scene.getRegistry().storage<ecs::Quad>();
    auto& meshStorage     = scene.getRegistry().storage<ecs::MeshRef>();
    auto& cameraStorage   = scene.getRegistry().storage<ecs::CameraObject>();
    auto& transformStorage = scene.getRegistry().storage<ecs::Transform>();

    for (size_t i = 0; i < scene.getEntities().size(); i++) {
        const ecs::Entity& e = scene.getEntities()[i];
        if (!transformStorage.has(e)) continue;

        auto& transform = transformStorage.get(e);

        if (sphereStorage.has(e)) {
            auto& sphere = sphereStorage.get(e);
            t = raySphereIntersection(ray, transform.position, sphere.radius);
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
