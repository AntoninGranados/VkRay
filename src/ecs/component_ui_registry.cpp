#include "./component_ui_registry.hpp"

#include "../scene/asset/mesh.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <limits>

namespace ecs {

ComponentUiRegistry& ComponentUiRegistry::get() {
    static ComponentUiRegistry r;
    return r;
}

void ComponentUiRegistry::init() {
    static bool init = false;
    if (init) return;
    init = true;

    auto& ui_reg = ComponentUiRegistry::get();

    ui_reg.add<ecs::Name>([](ecs::Name& n, ecs::Registry& r, ecs::Entity e){
        if (!ImGui::CollapsingHeader("Name")) return false;

        n.value.resize(128);

        ImGui::PushItemWidth(-FLT_MIN);
        ImGui::Text("Value:");
        if (ImGui::InputText("##Value", n.value.data(), 128)) {
            n.value = std::string(n.value.c_str());
        }
        ImGui::PopItemWidth();

        return false;
    });

    ui_reg.add<ecs::Sphere>([](ecs::Sphere& s, ecs::Registry& r, ecs::Entity e){
        bool update = false;
        
        if (!ImGui::CollapsingHeader("Sphere")) return false;

        ImGui::PushItemWidth(-FLT_MIN);
        ImGui::Text("Radius:");
        update |= ImGui::DragFloat("##Radius", &s.radius, 0.01f, 0.0f, FLT_MAX);
        ImGui::PopItemWidth();

        return update;
    });
    
    ui_reg.add<ecs::Plane>([](ecs::Plane& p, ecs::Registry& r, ecs::Entity e){
        ImGui::CollapsingHeader("Plane", ImGuiTreeNodeFlags_Bullet);
        return false;
    });
    
    ui_reg.add<ecs::Box>([](ecs::Box& p, ecs::Registry& r, ecs::Entity e){
        ImGui::CollapsingHeader("Box", ImGuiTreeNodeFlags_Bullet);
        return false;
    });
    
    ui_reg.add<ecs::MeshRef>([](ecs::MeshRef& ref, ecs::Registry& r, ecs::Entity e){
        auto* meshes = ComponentUiRegistry::get().meshAssets;
        if (!meshes || meshes->empty()) return false;

        if (!ImGui::CollapsingHeader("Mesh")) return false;
        ImGui::PushItemWidth(-FLT_MIN);
        
        bool update = false;
        int current = ref.handle;
        const std::string& currentName = (*meshes)[current].getName();
        const char* preview = currentName.empty() ? "Mesh" : currentName.c_str();
        if (ImGui::BeginCombo("##Mesh", preview)) {
            for (int i = 0; i < static_cast<int>(meshes->size()); i++) {
                const std::string& meshName = (*meshes)[i].getName();
                const char* display = meshName.empty() ? "Mesh" : meshName.c_str();
                std::string label = std::string(display) + "##MeshItem" + std::to_string(i);
                const bool selected = (i == current);
                if (ImGui::Selectable(label.c_str(), selected)) {
                    ref.handle = i;
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

        return update;
    });

    ui_reg.add<ecs::CameraObject>([](ecs::CameraObject& c, ecs::Registry& r, ecs::Entity e){
        bool update = false;
        
        if (!ImGui::CollapsingHeader("Camera")) return false;

        ImGui::PushItemWidth(-FLT_MIN);
        ImGui::Text("FOV:");
        update |= ImGui::DragFloat("##FOV", &c.fov, 0.1f, 1.0f, 160.0f);
        ImGui::Text("Aperture:");
        update |= ImGui::DragFloat("##Aperture", &c.aperture, 0.01f, 0.0f, 10.0f);
        ImGui::Text("Focus Depth:");
        update |= ImGui::DragFloat("##FocusDepth", &c.focusDepth, 0.01f, 0.0f, FLT_MAX);
        ImGui::PopItemWidth();

        return update;
    });

    ui_reg.add<ecs::Transform>([](ecs::Transform& t, ecs::Registry& r, ecs::Entity e){
        bool update = false;

        if (!ImGui::CollapsingHeader("Transform")) return false;

        ImGui::PushItemWidth(-FLT_MIN);

        if (t.positionToggled) {
            ImGui::Text("Position:");
            if (ImGui::DragFloat3("##Position", glm::value_ptr(t.position), 0.01f)) {
                t.updated = true;
                update = true;
            }
        } else {
            ImGui::TextDisabled("Position");
        }
        
        if (t.rotationToggled) {
            ImGui::Text("Rotation (Euler):");
            glm::vec3 euler = glm::degrees(glm::eulerAngles(t.rotation));
            if (ImGui::DragFloat3("##Rotation", glm::value_ptr(euler), 0.1f)) {
                t.rotation = glm::quat(glm::radians(euler));
                t.updated = true;
                update = true;
            }
        } else {
            ImGui::TextDisabled("Rotation");
        }
        
        if (t.scaleToggled) {
            ImGui::Text("Scale:");
            if (ImGui::DragFloat3("##Scale", glm::value_ptr(t.scale), 0.01f)) {
                t.updated = true;
                update = true;
            }
        } else {
            ImGui::TextDisabled("Scale");
        }
        
        ImGui::PopItemWidth();

        return update;
    });

    ui_reg.add<ecs::MaterialRef>([](ecs::MaterialRef& ref, ecs::Registry& r, ecs::Entity e){
        auto* mats = ComponentUiRegistry::get().materials;
        if (!mats || mats->empty()) return false;

        if (!ImGui::CollapsingHeader("Material")) return false;
        ImGui::PushItemWidth(-FLT_MIN);
        
        bool update = false;
        int current = ref.handle;
        const char* preview = (*mats)[current].name.empty() ? "Material" : (*mats)[current].name.c_str();
        if (ImGui::BeginCombo("##Material", preview)) {
            for (int i = 0; i < static_cast<int>(mats->size()); i++) {
                const char* display = (*mats)[i].name.empty() ? "Material" : (*mats)[i].name.c_str();
                std::string label = std::string(display) + "##MaterialItem" + std::to_string(i);
                const bool selected = (i == current);
                if (ImGui::Selectable(label.c_str(), selected)) {
                    ref.handle = i;
                    update = true;
                }
                if (selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();

        ImGui::BeginChild("MaterialData", ImVec2{0, 0}, ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_None);
        update |= drawMaterialUI((*mats)[current]);
        ImGui::EndChild();

        return update;
    });
}

} // namespace ecs
