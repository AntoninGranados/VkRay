#include "component_ui_registry.hpp"

#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "FontAwesome/IconsFontAwesome7.h"

#include "core/ecs/components.hpp"
#include "core/scene/asset/mesh.hpp"
#include "editor/field_ui.hpp"
#include "editor/scene/material_ui.hpp"
#include "editor/ui_utils.hpp"

namespace ecs {

ComponentUiRegistry& ComponentUiRegistry::get() {
    static ComponentUiRegistry r;
    return r;
}

bool ComponentUiRegistry::drawField(Component& component, const ecs::ComponentField& schema) {
    return ui::drawField(component.getField(schema.getId()), "##" + schema.getId());
}

void ComponentUiRegistry::add(const ecs::ComponentType& componentType) {
    drawers.emplace_back([type = componentType](Registry& registry, Entity e) {
        if (!registry.has(e, type)) return false;

        Component& component = registry.get(e, type);
        const std::string header = component.getType().getIcon() + " " + component.getType().getLabel();

        ComponentUiRegistry::beginDraw(&component, [&]() {
            registry.remove(e, type);
        });
        bool update = false;
        if (ImGui::CollapsingHeader(header.c_str(), component.getType().getFields().size() > 0 ? ImGuiTreeNodeFlags_None : ImGuiTreeNodeFlags_Bullet)) {
            for (const ecs::ComponentField& schema : component.getType().getFields()) {
                if (schema.isPrivate()) continue;
                if (schema.isAnimatable()) ui::drawKeyframeButton(e, component, schema.getId());
                update |= ComponentUiRegistry::drawField(component, schema);
            }
        }
        ComponentUiRegistry::endDraw();
        return update;
    });
}

void ComponentUiRegistry::init() {
    static bool init = false;
    if (init) return;
    init = true;

    auto& ui_reg = ComponentUiRegistry::get();

    ui_reg.add(ecs::Name);

    ui_reg.add(ecs::Sphere);
    ui_reg.add(ecs::Plane);
    ui_reg.add(ecs::Box);
    ui_reg.add(ecs::Collider);
    ui_reg.add(ecs::RigidBody);

    ui_reg.add(MeshRef, [](Component& c, Registry& r, Entity e) {
        auto* meshes = ComponentUiRegistry::get().meshAssets;
        if (!meshes || meshes->empty()) return false;

        bool update = false;
        if (ImGui::CollapsingHeader(ICON_FA_CUBE " Mesh")) {
            ImGui::PushItemWidth(-FLT_MIN);

            int current = c.get<int>("handle");
            const std::string& currentName = (*meshes)[current].getName();
            const char* preview = currentName.empty() ? "Mesh" : currentName.c_str();
            if (ImGui::BeginCombo("##Mesh", preview)) {
                for (int i = 0; i < static_cast<int>(meshes->size()); i++) {
                    const std::string& meshName = (*meshes)[i].getName();
                    const char* display = meshName.empty() ? "Mesh" : meshName.c_str();
                    std::string label = std::string(display) + "##MeshItem" + std::to_string(i);
                    const bool selected = (i == current);
                    if (ImGui::Selectable(label.c_str(), selected)) {
                        c.set<int>("handle", i);
                        update = true;
                    }
                    if (selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();

            ImGui::BeginChild("MeshData", ImVec2{0, 0}, ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_None);
            update |= drawMeshAssetUI((*meshes)[current]);
            ImGui::EndChild();
        }
        return update;
    });

    ui_reg.add(Camera, [](Component& c, Registry& r, Entity e) {
        bool update = false;
        if (ImGui::CollapsingHeader(ICON_FA_VIDEO " Camera")) {
            for (const ecs::ComponentField& schema : c.getType().getFields()) {
                if (schema.isPrivate()) continue;
                if (schema.isAnimatable()) ui::drawKeyframeButton(e, c, schema.getId());
                update |= drawField(c, schema);
            }
            if (ImGui::Button("Set as preview", ImVec2{ -FLT_MIN, 0 })) {
                auto& allCameras = r.storage(Camera);
                for (const auto& other : allCameras.entities()) {
                    allCameras.get(other).set<bool>("is_preview", false);
                }
                c.set<bool>("is_preview", true);
                Core::getScene().getCamera().setPreviewCamera(e);
                update = true;
            }
        }

        return update;
    });

    ui_reg.add(ecs::ThinLensCamera);
    ui_reg.add(ecs::Transform);

    ui_reg.add(MaterialRef, [](Component& c, Registry& r, Entity e) {
        auto* mats = ComponentUiRegistry::get().materials;
        if (!mats || mats->empty()) return false;

        bool update = false;
        if (ImGui::CollapsingHeader(ICON_FA_PALETTE " Material")) {
            ImGui::PushItemWidth(-FLT_MIN);

            int current = c.get<int>("handle");
            const char* preview = (*mats)[current].getName().empty() ? "Material" : (*mats)[current].getName().c_str();
            if (ImGui::BeginCombo("##Material", preview)) {
                for (int i = 0; i < static_cast<int>(mats->size()); i++) {
                    const char* display = (*mats)[i].getName().empty() ? "Material" : (*mats)[i].getName().c_str();
                    std::string label = std::string(display) + "##MaterialItem" + std::to_string(i);
                    const bool selected = (i == current);
                    if (ImGui::Selectable(label.c_str(), selected)) {
                        c.set<int>("handle", i);
                        update = true;
                    }
                    if (selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();

            ImGui::BeginChild("MaterialData", ImVec2{0, 0}, ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_None);
            update |= drawMaterialUI(current);
            ImGui::EndChild();
        }

        return update;
    });
}

} // namespace ecs
