#include "component_ui_registry.hpp"

#include <algorithm>
#include <format>
#include <utility>

#include "FontAwesome/IconsFontAwesome7.h"

#include "core/core.hpp"
#include "core/ecs/systems/mesh_system.hpp"
#include "core/render/camera_lens_table.hpp"
#include "core/render/compositing_table.hpp"
#include "core/render/material_table.hpp"
#include "core/render/sky_table.hpp"
#include "core/scene/asset/mesh.hpp"
#include "core/scene/scene.hpp"
#include "core/shader_plugin/shader_plugin.hpp"
#include "editor/editor.hpp"
#include "editor/fields/field_ui.hpp"
#include "editor/ui_utils.hpp"

namespace ecs {

static bool drawPluginErrorAndFields(ShaderPlugin& plugin, const std::string& idSuffix) {
    if (!plugin.getError().empty()) ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "%s", plugin.getError().c_str());

    return ui::drawGroupedFields(plugin.getComponent().getFields(), idSuffix);
}

static bool drawShaderPluginField(Component& c, const std::string& type, int version,
                                  int (*slotFor)(const std::filesystem::path&)) {
    const std::string header = c.getType().getIcon() + " " + c.getType().getLabel();
    if (!ImGui::CollapsingHeader(header.c_str())) return false;

    bool update = ui::drawField(c.getField("path"), "##path");

    ShaderPlugin& plugin = c.payload<ShaderPlugin>("plugin");
    const std::filesystem::path path = c.get<std::filesystem::path>("path");
    plugin.parse(path, type, version, slotFor(path));

    update |= drawPluginErrorAndFields(plugin, std::format("##{}", header));
    return update;
}

ComponentUiRegistry& ComponentUiRegistry::get() {
    static ComponentUiRegistry r;
    return r;
}

void ComponentUiRegistry::add(const ecs::ComponentType& componentType) {
    addWithFields(componentType, [](Component&, Registry&, Entity) { return false; }, true);
}

void ComponentUiRegistry::add(const ecs::ComponentType& type,
                              std::function<bool(Component&, Registry&, Entity)> extra) {
    addWithFields(type, std::move(extra), false);
}

void ComponentUiRegistry::addWithFields(const ecs::ComponentType& type,
                                        std::function<bool(Component&, Registry&, Entity)> extra, bool bulletIfEmpty) {
    drawers.emplace_back([extra, type, bulletIfEmpty](Registry& registry, Entity e) {
        if (!registry.has(e, type)) return;

        Component& component = registry.get(e, type);
        const std::string header = std::format("{} {}", component.getType().getIcon(), component.getType().getLabel());
        auto& fields = component.getFields();

        bool remove = ComponentUiRegistry::beginDraw(&component, type);
        if (remove) registry.remove(e, type);
        bool update = false;
        const bool useBullet = bulletIfEmpty && fields.empty();
        if (!remove &&
            ImGui::CollapsingHeader(header.c_str(), useBullet ? ImGuiTreeNodeFlags_Bullet : ImGuiTreeNodeFlags_None)) {
            update |= ui::drawGroupedFields(fields, std::format("##{}", header));
            update |= extra(component, registry, e);
        }
        ComponentUiRegistry::endDraw();
        if (update) registry.markChanged(type);
    });
}

void ComponentUiRegistry::addCustom(const ecs::ComponentType& type,
                                    std::function<bool(Component&, Registry&, Entity)> custom) {
    drawers.emplace_back([custom, type](Registry& registry, Entity e) {
        if (!registry.has(e, type)) return;

        Component& component = registry.get(e, type);
        bool remove = ComponentUiRegistry::beginDraw(&component, type);
        if (remove) registry.remove(e, type);
        bool update = !remove && custom(component, registry, e);
        ComponentUiRegistry::endDraw();
        if (update) registry.markChanged(type);
    });
}

void ComponentUiRegistry::init() {
    static bool init = false;
    if (init) return;
    init = true;

    auto& ui_reg = ComponentUiRegistry::get();

    ui_reg.add(ecs::Name);
    ui_reg.add(ecs::Transform);

    ui_reg.add(ecs::Sphere);
    ui_reg.add(ecs::Plane);
    ui_reg.add(ecs::Box);
    ui_reg.add(ecs::Quad);

    ui_reg.add(ecs::Mesh, [](Component&, Registry&, Entity e) {
        if (const MeshAsset* mesh = Core::getScene().getMeshAsset(e)) {
            ImGui::BeginChild("MeshData", ImVec2{0, 0}, ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY,
                              ImGuiWindowFlags_None);
            ImGui::Text("Vertices: %zu", mesh->getVertices().size());
            ImGui::Text("Faces:    %zu", mesh->getIndices().size() / 3);
            ImGui::EndChild();
        }
        return false;
    });

    ui_reg.addCustom(ecs::MeshSimplify, [](Component& c, Registry& r, Entity e) {
        bool update = false;
        if (ImGui::CollapsingHeader(ICON_FA_CUBE " Mesh Simplify")) {
            float ratio = c.get<float>("ratio");
            ImGui::PushItemWidth(-FLT_MIN);
            if (ImGui::SliderFloat("##Ratio", &ratio, 0.05f, 1.0f, "%.2f")) {
                c.set<float>("ratio", ratio);
                ecs::requestMeshSimplify(r, e, ratio);
                update = true;
            }
            ImGui::PopItemWidth();
            if (ImGui::BeginTable("##SimplifyButtons", 2, ImGuiTableFlags_SizingStretchSame)) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                if (ImGui::Button("Apply", ImVec2(-FLT_MIN, 0.0f))) {
                    ecs::applyMeshSimplification(r, e);
                    update = true;
                }
                ImGui::TableSetColumnIndex(1);
                ui::PushCancelStyleColor();
                if (ImGui::Button("Revert", ImVec2(-FLT_MIN, 0.0f))) {
                    ecs::revertMeshSimplification(r, e);
                    update = true;
                }
                ui::PopCancelStyleColor();
                ImGui::EndTable();
            }
        }
        return update;
    });

    ui_reg.add(ecs::MeshRef);

    ui_reg.add(ecs::Collider);
    ui_reg.add(ecs::RigidBody);

    ui_reg.add(ecs::Camera);
    ui_reg.add(ecs::TiltShiftLens);
    ui_reg.add(ecs::GeometricAperture);
    ui_reg.add(ecs::ImageAperture);
    ui_reg.addCustom(ecs::CameraLensPlugin, [](Component& c, Registry&, Entity) {
        return drawShaderPluginField(c, CameraLensTable::kType, CameraLensTable::kVersion, CameraLensTable::slotFor);
    });

    ui_reg.add(ecs::Material, [](Component&, Registry&, Entity e) {
        Editor::getMaterialPreview().drawPreview(e);
        return false;
    });

    ui_reg.add(ecs::MaterialRef, [](Component& c, Registry&, Entity) {
        const ecs::Entity handle = c.get<ecs::Entity>("handle");
        if (handle != ecs::Entity{}) Editor::getMaterialPreview().drawPreview(handle);
        return false;
    });

    ui_reg.add(ecs::Diffuse);
    ui_reg.add(ecs::Emissive);
    ui_reg.add(ecs::Metal);
    ui_reg.add(ecs::Glossy);
    ui_reg.add(ecs::Dielectric);
    ui_reg.add(ecs::Volume);
    ui_reg.add(ecs::Principled);
    ui_reg.addCustom(ecs::MaterialPlugin, [](Component& c, Registry&, Entity) {
        return drawShaderPluginField(c, MaterialTable::kType, MaterialTable::kVersion, MaterialTable::slotFor);
    });

    ui_reg.add(ecs::Environment);
    ui_reg.add(ecs::Physics, [](Component&, Registry&, Entity) {
        Scene& scene = Core::getScene();
        if (scene.isPhysicsBakeInProgress()) {
            const int total = std::max(1, scene.getPhysicsBakeTotalFrames());
            const int current = std::clamp(scene.getPhysicsBakeCurrentFrame(), 0, total);
            const float progress = float(current) / float(total);
            ImGui::ProgressBar(progress, ImVec2(-FLT_MIN, 0.0f), std::format("{:.0f}%", progress * 100.0f).c_str());
        } else if (ImGui::Button(ICON_FA_HARD_DRIVE " Bake Physics", ImVec2(-FLT_MIN, 0.0f))) {
            scene.bakePhysics();
        }
        return false;
    });
    ui_reg.addCustom(ecs::SkyPlugin, [](Component& c, Registry&, Entity) {
        return drawShaderPluginField(c, SkyTable::kType, SkyTable::kVersion, SkyTable::slotFor);
    });

    ui_reg.addCustom(ecs::Compositing, [](Component& c, Registry&, Entity) {
        const std::string header = c.getType().getIcon() + " " + c.getType().getLabel();
        if (!ImGui::CollapsingHeader(header.c_str())) return false;

        bool update = false;
        CompositingPasses& list = c.payload<CompositingPasses>("passes");
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
            if (ImGui::Selectable(std::format("{}##{}", list.passes[i].name, i).c_str(),
                                  selected == static_cast<int>(i)))
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
                if (auto path = ui::openFileDialog({{"Compositing Pass Shader", "glsl"}}, "assets/compositing/")) {
                    pass.path = *path;
                    pass.plugin->parse(pass.path, CompositingTable::kType, CompositingTable::kVersion,
                                       CompositingTable::slotFor(pass.path));
                    update = true;
                }
            }

            update |= drawPluginErrorAndFields(*pass.plugin, std::format("##CompositingPass{}", selected));
        }

        storage->SetInt(selectedId, selected);
        return update;
    });
}

} // namespace ecs
