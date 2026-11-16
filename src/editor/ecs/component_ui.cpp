#include "component_ui.hpp"

#include <algorithm>
#include <format>
#include <utility>

#include "FontAwesome/IconsFontAwesome7.h"
#include "imgui/imgui.h"

#include "core/core.hpp"
#include "core/ecs/systems/mesh_system.hpp"
#include "core/ecs/systems/physics/physics_system.hpp"
#include "core/render/script_table.hpp"
#include "core/scene/asset/mesh.hpp"
#include "core/scene/scene.hpp"
#include "editor/editor.hpp"
#include "editor/fields/field_ui.hpp"
#include "editor/ui_utils.hpp"

namespace ecs {

void ComponentUi::draw(Registry& registry, Entity entity) {
    static const ComponentType* const kOrder[] = {&Name,
                                                  &Transform,
                                                  &Sphere,
                                                  &Plane,
                                                  &Box,
                                                  &Quad,
                                                  &Mesh,
                                                  &MeshSimplify,
                                                  &MeshRef,
                                                  &Collider,
                                                  &RigidBody,
                                                  &Camera,
                                                  &GeometricAperture,
                                                  &ImageAperture,
                                                  &CameraLensPlugin,
                                                  &Material,
                                                  &MaterialRef,
                                                  &MaterialPlugin,
                                                  &Environment,
                                                  &Physics,
                                                  &SkyPlugin,
                                                  &Compositing};

    for (const ComponentType* type : kOrder) {
        if (!registry.has(entity, *type)) continue;

        Component& component = registry.get(entity, *type);
        ImGui::PushID(&component);
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0, 0, 0, 0.2));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0, 0, 0, 0));
        ImGui::BeginChild("Component", ImVec2{0, 0}, ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY,
                          ImGuiWindowFlags_None);

        ImGui::BeginDisabled(type->isPermanent());
        const bool remove = ui::minusButton("Remove");
        ImGui::EndDisabled();
        ImGui::SameLine();

        if (remove) registry.remove(entity, *type);
        const bool update = !remove && drawComponent(component, registry, entity);

        ImGui::EndChild();
        ImGui::PopStyleColor(3);
        ImGui::PopID();
        if (update) registry.markChanged(*type);
    }
    registry.flush();
}

bool ComponentUi::drawComponent(Component& component, Registry& registry, Entity entity) {
    const ComponentType& type = component.getType();
    if (type == MeshSimplify) return drawMeshSimplify(component, registry, entity);
    if (type == Compositing) return drawCompositing(component);
    for (const ScriptKind* kind : ScriptTable::componentKinds())
        if (type == *kind->component) return drawShaderPluginField(component, *kind);

    const std::string header = std::format("{} {}", type.getIcon(), type.getLabel());
    const bool hasExtra = type == Mesh || type == Material || type == MaterialRef || type == Physics;
    const bool useBullet = !hasExtra && component.getFields().empty();
    if (!ImGui::CollapsingHeader(header.c_str(), useBullet ? ImGuiTreeNodeFlags_Bullet : ImGuiTreeNodeFlags_None))
        return false;

    const bool update = ui::drawGroupedFields(component.getFields(), std::format("##{}", header));

    if (type == Mesh) {
        if (const MeshAsset* mesh = Core::getScene().getMeshAsset(entity)) {
            ImGui::BeginChild("MeshData", ImVec2{0, 0}, ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY,
                              ImGuiWindowFlags_None);
            ImGui::Text("Vertices: %zu", mesh->getVertices().size());
            ImGui::Text("Faces:    %zu", mesh->getIndices().size() / 3);
            ImGui::EndChild();
        }
    } else if (type == Material) {
        Editor::getMaterialPreview().drawPreview(entity);
    } else if (type == MaterialRef) {
        const Entity handle = component.get<Entity>("handle");
        if (handle != Entity{}) Editor::getMaterialPreview().drawPreview(handle);
    } else if (type == Physics) {
        Scene& scene = Core::getScene();
        const PhysicsBakeState& bakeState = scene.getPhysicsBakeState();
        if (bakeState.inProgress) {
            const int total = std::max(1, bakeState.totalFrames);
            const int current = std::clamp(bakeState.nextFrame, 0, total);
            const float progress = float(current) / float(total);
            ImGui::ProgressBar(progress, ImVec2(-FLT_MIN, 0.0f), std::format("{:.0f}%", progress * 100.0f).c_str());
        } else if (ImGui::Button(ICON_FA_HARD_DRIVE " Bake Physics", ImVec2(-FLT_MIN, 0.0f))) {
            bakePhysicsSimulation(scene);
        }
    }
    return update;
}

bool ComponentUi::drawMeshSimplify(Component& component, Registry& registry, Entity entity) {
    bool update = false;
    if (ImGui::CollapsingHeader(ICON_FA_CUBE " Mesh Simplify")) {
        float ratio = component.get<float>("ratio");
        ImGui::PushItemWidth(-FLT_MIN);
        if (ImGui::SliderFloat("##Ratio", &ratio, 0.05f, 1.0f, "%.2f")) {
            component.set<float>("ratio", ratio);
            requestMeshSimplify(registry, entity, ratio);
            update = true;
        }
        ImGui::PopItemWidth();
        if (ImGui::BeginTable("##SimplifyButtons", 2, ImGuiTableFlags_SizingStretchSame)) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            if (ImGui::Button("Apply", ImVec2(-FLT_MIN, 0.0f))) {
                applyMeshSimplification(registry, entity);
                update = true;
            }
            ImGui::TableSetColumnIndex(1);
            ui::PushCancelStyleColor();
            if (ImGui::Button("Revert", ImVec2(-FLT_MIN, 0.0f))) {
                revertMeshSimplification(registry, entity);
                update = true;
            }
            ui::PopCancelStyleColor();
            ImGui::EndTable();
        }
    }
    return update;
}

bool ComponentUi::drawCompositing(Component& component) {
    const std::string header = component.getType().getIcon() + " " + component.getType().getLabel();
    if (!ImGui::CollapsingHeader(header.c_str())) return false;

    bool update = false;
    CompositingPasses& list = component.payload<CompositingPasses>("passes");
    ImGuiStorage* storage = ImGui::GetStateStorage();
    const ImGuiID selectedId = ImGui::GetID("##CompositingSelectedPass");
    int selected = storage->GetInt(selectedId, -1);

    ImGui::BeginChild("##CompositingPassList", ImVec2(0, 120), ImGuiChildFlags_Borders);
    ImGui::PushStyleColor(ImGuiCol_Header,
                          ImVec4(ui::kDraculaPurple.x, ui::kDraculaPurple.y, ui::kDraculaPurple.z, 0.35f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered,
                          ImVec4(ui::kDraculaPurple.x, ui::kDraculaPurple.y, ui::kDraculaPurple.z, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ui::kDraculaPurple);
    for (size_t i = 0; i < list.passes.size(); i++)
        if (ImGui::Selectable(std::format("{}##{}", list.passes[i].name, i).c_str(), selected == static_cast<int>(i)))
            selected = static_cast<int>(i);
    ImGui::PopStyleColor(3);
    ImGui::EndChild();

    if (ui::plusButton("AddCompositingPass")) {
        CompositingPassEntry pass;
        pass.name = std::format("Pass {}", list.passes.size() + 1);
        list.passes.push_back(std::move(pass));
        selected = static_cast<int>(list.passes.size()) - 1;
        update = true;
    }
    ImGui::SameLine();

    const bool hasSelection = selected >= 0 && selected < static_cast<int>(list.passes.size());
    if (!hasSelection) ImGui::BeginDisabled();
    if (ui::minusButton("RemoveCompositingPass")) {
        list.passes.erase(list.passes.begin() + selected);
        selected = -1;
        update = true;
    }
    ImGui::SameLine();
    if (ui::upButton("MoveCompositingPassUp") && selected > 0) {
        std::swap(list.passes[selected], list.passes[selected - 1]);
        selected--;
        update = true;
    }
    ImGui::SameLine();
    if (ui::downButton("MoveCompositingPassDown") && selected < static_cast<int>(list.passes.size()) - 1) {
        std::swap(list.passes[selected], list.passes[selected + 1]);
        selected++;
        update = true;
    }
    if (!hasSelection) ImGui::EndDisabled();

    if (selected >= 0 && selected < static_cast<int>(list.passes.size())) {
        CompositingPassEntry& pass = list.passes[selected];
        ImGui::Separator();

        std::string name = pass.name;
        name.resize(128, '\0');
        if (ImGui::InputText("##CompositingPassName", name.data(), name.size())) {
            pass.name = name.c_str();
            update = true;
        }

        ImGui::TextUnformatted(pass.path.empty() ? "(no script)" : pass.path.filename().string().c_str());
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_FOLDER_OPEN "##BrowseCompositingPassScript")) {
            if (auto path = ui::openFileDialog({{"Compositing Pass Shader", "slang"}}, "assets/compositing/")) {
                pass.path = *path;
                pass.plugin->parse(pass.path, ScriptTable::kCompositing);
                update = true;
            }
        }

        update |= drawPluginErrorAndFields(*pass.plugin, std::format("##CompositingPass{}", selected));
    }

    storage->SetInt(selectedId, selected);
    return update;
}

bool ComponentUi::drawShaderPluginField(Component& component, const ScriptKind& kind) {
    const std::string header = component.getType().getIcon() + " " + component.getType().getLabel();
    if (!ImGui::CollapsingHeader(header.c_str())) return false;

    bool update = ui::drawField(component.getField("path"), "##path");

    ShaderPlugin& plugin = component.payload<ShaderPlugin>("plugin");
    plugin.parse(component.get<std::filesystem::path>("path"), kind);

    update |= drawPluginErrorAndFields(plugin, std::format("##{}", header));
    return update;
}

bool ComponentUi::drawPluginErrorAndFields(ShaderPlugin& plugin, const std::string& idSuffix) {
    if (!plugin.getError().empty()) ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "%s", plugin.getError().c_str());

    return ui::drawGroupedFields(plugin.getComponent().getFields(), idSuffix);
}

} // namespace ecs
