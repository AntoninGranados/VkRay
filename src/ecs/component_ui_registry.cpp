#include "./component_ui_registry.hpp"

#include <glm/gtc/type_ptr.hpp>

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
        n.value.resize(128);

        ImGui::SeparatorText("Name");

        ImGui::PushItemWidth(-FLT_MIN);
        ImGui::Text("Value:");
        if (ImGui::InputText("##Value", n.value.data(), 128))
        ImGui::PopItemWidth();

        n.value = std::string(n.value.c_str());
        return false;
    });

    ui_reg.add<ecs::Sphere>([](ecs::Sphere& s, ecs::Registry& r, ecs::Entity e){
        bool update = false;

        ImGui::SeparatorText("Sphere");

        ImGui::PushItemWidth(-FLT_MIN);
        ImGui::Text("Radius:");
        update |= ImGui::DragFloat("##Radius", &s.radius, 0.01f, 0.0f, FLT_MAX);
        ImGui::PopItemWidth();

        return update;
    });
    
    ui_reg.add<ecs::Plane>([](ecs::Plane& p, ecs::Registry& r, ecs::Entity e){
        ImGui::SeparatorText("Plane");
        return false;
    });
    
    ui_reg.add<ecs::Box>([](ecs::Box& p, ecs::Registry& r, ecs::Entity e){
        ImGui::SeparatorText("Box");
        return false;
    });

    ui_reg.add<ecs::Transform>([](ecs::Transform& t, ecs::Registry& r, ecs::Entity e){
        bool update = false;
        
        ImGui::SeparatorText("Transform");

        ImGui::PushItemWidth(-FLT_MIN);

        if (t.positionToggled) {
            ImGui::Text("Position:");
            update |= ImGui::DragFloat3("##Position", glm::value_ptr(t.position), 0.01f);
        } else {
            ImGui::TextDisabled("Position");
        }
        
        if (t.rotationToggled) {
            ImGui::Text("Rotation:");
            update |= ImGui::DragFloat4("##Rotation", glm::value_ptr(t.rotation), 0.01f);
        } else {
            ImGui::TextDisabled("Rotation");
        }
        
        if (t.scaleToggled) {
            ImGui::Text("Scale:");
            update |= ImGui::DragFloat3("##Scale", glm::value_ptr(t.scale), 0.01f);
        } else {
            ImGui::TextDisabled("Scale");
        }
        
        ImGui::PopItemWidth();

        t.updated = update;
        t.updateLocal();
        return update;
    });
}

} // namespace ecs

