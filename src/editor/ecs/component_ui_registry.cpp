#include "component_ui_registry.hpp"

#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "FontAwesome/IconsFontAwesome7.h"

#include "core/ecs/components.hpp"
#include "core/scene/asset/mesh.hpp"
#include "editor/scene/material_ui.hpp"
#include "editor/ui_utils.hpp"

namespace ecs {

ComponentUiRegistry& ComponentUiRegistry::get() {
    static ComponentUiRegistry r;
    return r;
}

bool ComponentUiRegistry::drawField(Component& component, const ecs::Field& schema) {
    bool update = false;
    float step = schema.metadata.step != 0.0f ? schema.metadata.step : 0.01f;
    float fmin = schema.metadata.min;
    float fmax = schema.metadata.max;
    std::string label = "##" + schema.id;

    ImGui::Text("%s:", schema.label.c_str());
    switch (schema.type) {
        case ecs::FieldType::Float:
            update |= ImGui::DragFloat(label.c_str(), component.getPtr<float>(schema.id), step, fmin, fmax);
            break;
        case ecs::FieldType::Float2:
            update |= ImGui::DragFloat2(label.c_str(), component.getPtr<float>(schema.id), step, fmin, fmax);
            break;
        case ecs::FieldType::Float3:
            update |= ImGui::DragFloat3(label.c_str(), component.getPtr<float>(schema.id), step, fmin, fmax);
            break;
        case ecs::FieldType::Float4:
            update |= ImGui::DragFloat4(label.c_str(), component.getPtr<float>(schema.id), step, fmin, fmax);
            break;
        case ecs::FieldType::Int:
            update |= ImGui::DragInt(label.c_str(), component.getPtr<int>(schema.id), step, (int)fmin, (int)fmax);
            break;
        case ecs::FieldType::Int2:
            update |= ImGui::DragInt2(label.c_str(), component.getPtr<int>(schema.id), step, (int)fmin, (int)fmax);
            break;
        case ecs::FieldType::Int3:
            update |= ImGui::DragInt3(label.c_str(), component.getPtr<int>(schema.id), step, (int)fmin, (int)fmax);
            break;
        case ecs::FieldType::Int4:
            update |= ImGui::DragInt4(label.c_str(), component.getPtr<int>(schema.id), step, (int)fmin, (int)fmax);
            break;
        case ecs::FieldType::Bool:
            update |= ImGui::Checkbox(label.c_str(), component.getPtr<bool>(schema.id));
            break;
        case ecs::FieldType::String:
            update |= ImGui::InputText(label.c_str(), component.getPtr<char>(schema.id), ecs::maxStringFieldSize);
            break;
        case ecs::FieldType::Quat: {
            glm::quat* q = component.getPtr<glm::quat>(schema.id);
            glm::vec3 euler = glm::degrees(glm::eulerAngles(*q));
            if (ImGui::DragFloat3(label.c_str(), glm::value_ptr(euler), step)) {
                *q = glm::quat(glm::radians(euler));
                update = true;
            }
            break;
        }
    }
    return update;
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
            ImGui::PushItemWidth(-FLT_MIN);
            for (const ecs::Field& schema : component.getType().getFields()) {
                if (schema.isPrivate) continue;
                if (schema.metadata.animatable) ui::drawKeyframeButton(e, component, schema);
                update |= ComponentUiRegistry::drawField(component, schema);
            }
            ImGui::PopItemWidth();
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
            ImGui::PushItemWidth(-FLT_MIN);
            for (const ecs::Field& schema : c.getType().getFields()) {
                if (schema.isPrivate) continue;
                if (schema.metadata.animatable) ui::drawKeyframeButton(e, c, schema);
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
            ImGui::PopItemWidth();
        }

        return update;
    });

    ui_reg.add(ecs::Transform);

    ui_reg.add(MaterialRef, [](Component& c, Registry& r, Entity e) {
        auto* mats = ComponentUiRegistry::get().materials;
        if (!mats || mats->empty()) return false;

        bool update = false;
        if (ImGui::CollapsingHeader(ICON_FA_PALETTE " Material")) {
            ImGui::PushItemWidth(-FLT_MIN);

            int current = c.get<int>("handle");
            const char* preview = (*mats)[current].name.empty() ? "Material" : (*mats)[current].name.c_str();
            if (ImGui::BeginCombo("##Material", preview)) {
                for (int i = 0; i < static_cast<int>(mats->size()); i++) {
                    const char* display = (*mats)[i].name.empty() ? "Material" : (*mats)[i].name.c_str();
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
