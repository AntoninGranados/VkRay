#include "component_ui_registry.hpp"

#include <cmath>
#include <unordered_map>

#include <glm/gtc/type_ptr.hpp>
#include "FontAwesome/IconsFontAwesome7.h"

#include "core/ecs/components/animation/material_anim.hpp"
#include "core/scene/asset/mesh.hpp"
#include "core/animation_handler.hpp"
#include "editor/ui_constants.hpp"
#include "editor/scene/material_ui.hpp"

namespace ecs {

struct RotationEditorState {
    glm::quat sourceQuat { 1.0f, 0.0f, 0.0f, 0.0f };
    glm::vec3 eulerDeg { 0.0f, 0.0f, 0.0f };
    bool initialized = false;
};

RotationEditorState& rotationEditorState(const ecs::Entity& e) {
    static std::unordered_map<ecs::Entity, RotationEditorState> states;
    return states[e];
}

bool sameRotation(const glm::quat& a, const glm::quat& b) {
    const glm::quat qa = glm::normalize(a);
    const glm::quat qb = glm::normalize(b);
    return std::abs(glm::dot(qa, qb)) > 1.0f - 1e-5f;
}

ComponentUiRegistry& ComponentUiRegistry::get() {
    static ComponentUiRegistry r;
    return r;
}


void keyframeButton(ecs::Registry& r, ecs::Entity& e, ecs::TransformAnim& anim, const TransformKeyframeType& type, std::function<void(const bool&)> func) {
    bool hasKeyframe;
    switch (type) {
        case TransformKeyframeType::Position:   hasKeyframe = anim.hasPositionKeyframe(Core::getAnimation().getFrame());  break;
        case TransformKeyframeType::Rotation:   hasKeyframe = anim.hasRotationKeyframe(Core::getAnimation().getFrame());  break;
        case TransformKeyframeType::Scale:      hasKeyframe = anim.hasScaleKeyframe(Core::getAnimation().getFrame());     break;
    }

    if (hasKeyframe) ImGui::PushStyleColor(ImGuiCol_Text, ui::kKeyframeOnColor);
    else ImGui::PushStyleColor(ImGuiCol_Text, ui::kKeyframeOffColor);

    ImGui::PushID((long)&type + (long)&e);
    ui::PushTransparentStyleColor();
    if (ImGui::Button(ICON_FA_SQUARE "##KeyframePos")) {
        func(hasKeyframe);
    }
    ui::PopTransparentStyleColor();
    ImGui::PopID();

    ImGui::PopStyleColor();
    ImGui::SameLine();
};

void ComponentUiRegistry::init() {
    static bool init = false;
    if (init) return;
    init = true;

    auto& ui_reg = ComponentUiRegistry::get();

    ui_reg.add<ecs::Name>([](ecs::Name& n, ecs::Registry& r, ecs::Entity e){
        if (!ImGui::CollapsingHeader(ICON_FA_TAG " Name")) return false;

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

        if (!ImGui::CollapsingHeader(ICON_FA_CIRCLE " Sphere")) return false;

        ImGui::PushItemWidth(-FLT_MIN);
        ImGui::Text("Radius:");
        update |= ImGui::DragFloat("##Radius", &s.radius, 0.01f, 0.0f, FLT_MAX);
        ImGui::PopItemWidth();

        return update;
    });

    ui_reg.add<ecs::Plane>([](ecs::Plane& p, ecs::Registry& r, ecs::Entity e){
        ImGui::CollapsingHeader(ICON_FA_SQUARE " Plane", ImGuiTreeNodeFlags_Bullet);
        return false;
    });

    ui_reg.add<ecs::Collider>([](ecs::Collider& c, ecs::Registry& r, ecs::Entity e){
        bool update = false;
        if (!ImGui::CollapsingHeader(ICON_FA_SQUARE " Collider")) return false;

        ImGui::PushItemWidth(-FLT_MIN);
        ImGui::Text("Restitution:");
        update |= ImGui::DragFloat("##ColliderRestitution", &c.restitution, 0.01f, 0.0f, 1.0f);
        ImGui::Text("Friction:");
        update |= ImGui::DragFloat("##ColliderFriction", &c.friction, 0.01f, 0.0f, 1.0f);
        ImGui::PopItemWidth();

        return update;
    });

    ui_reg.add<ecs::Box>([](ecs::Box& p, ecs::Registry& r, ecs::Entity e){
        ImGui::CollapsingHeader(ICON_FA_BOX " Box", ImGuiTreeNodeFlags_Bullet);
        return false;
    });

    ui_reg.add<ecs::RigidBody>([](ecs::RigidBody& rb, ecs::Registry& r, ecs::Entity e){
        bool update = false;
        if (!ImGui::CollapsingHeader(ICON_FA_CUBES_STACKED " Rigid Body")) return false;

        ImGui::PushItemWidth(-FLT_MIN);
        update |= ImGui::Checkbox("Use Gravity##RigidBodyGravity", &rb.useGravity);
        ImGui::Text("Density:");
        update |= ImGui::DragFloat("##RigidBodyDensity", &rb.density, 1.0f, 0.1f, 10000.0f);
        ImGui::Text("Linear Velocity:");
        update |= ImGui::DragFloat3("##RigidBodyLinearVelocity", glm::value_ptr(rb.linearVelocity), 0.01f);
        ImGui::Text("Angular Velocity:");
        update |= ImGui::DragFloat3("##RigidBodyAngularVelocity", glm::value_ptr(rb.angularVelocity), 0.01f);
        ImGui::PopItemWidth();

        return update;
    });

    ui_reg.add<ecs::MeshRef>([](ecs::MeshRef& ref, ecs::Registry& r, ecs::Entity e){
        auto* meshes = ComponentUiRegistry::get().meshAssets;
        if (!meshes || meshes->empty()) return false;

        if (!ImGui::CollapsingHeader(ICON_FA_CUBE " Mesh")) return false;
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

        if (!ImGui::CollapsingHeader(ICON_FA_VIDEO " Camera")) return false;

        ImGui::PushItemWidth(-FLT_MIN);
        ImGui::Text("FOV:");
        update |= ImGui::DragFloat("##FOV", &c.fov, 0.1f, 1.0f, 160.0f);
        ImGui::Text("Aperture:");
        update |= ImGui::DragFloat("##Aperture", &c.aperture, 0.01f, 0.0f, 10.0f);
        ImGui::Text("Focus Depth:");
        update |= ImGui::DragFloat("##FocusDepth", &c.focusDepth, 0.01f, 0.0f, FLT_MAX);

        if (ImGui::Button("Set as preview", ImVec2{ -FLT_MIN, 0 })) {
            c.isPreview = true;
            c.previewJustSet = true;
            update = true;
        }
        if (update)
            c.updated = true;
        ImGui::PopItemWidth();

        return update;
    });

    ui_reg.add<ecs::Transform>([](ecs::Transform& t, ecs::Registry& r, ecs::Entity e){
        bool update = false;

        if (!ImGui::CollapsingHeader(ICON_FA_ARROWS_UP_DOWN_LEFT_RIGHT " Transform")) return false;

        bool isAnimated = r.has<ecs::TransformAnim>(e);

        ImGui::PushItemWidth(-FLT_MIN);

        if (isAnimated) {
            auto& anim = r.get<ecs::TransformAnim>(e);
            keyframeButton(
                r, e, anim, TransformKeyframeType::Position,
                [&](const bool& hasKeyframe) -> void {
                    if (!hasKeyframe) anim.insertPositionKeyframe(Core::getAnimation().getFrame(), t.position);
                    else anim.removePositionKeyframe(Core::getAnimation().getFrame());
                }
            );
        }
        ImGui::Text("Position:");
        if (ImGui::DragFloat3("##Position", glm::value_ptr(t.position), 0.01f)) {
            t.updated = true;
            update = true;
        }

        if (isAnimated) {
            auto& anim = r.get<ecs::TransformAnim>(e);
            keyframeButton(
                r, e, anim, TransformKeyframeType::Rotation,
                [&](const bool& hasKeyframe) -> void {
                    if (!hasKeyframe) anim.insertRotationKeyframe(Core::getAnimation().getFrame(), t.rotation);
                    else anim.removeRotationKeyframe(Core::getAnimation().getFrame());
                }
            );
        }
        ImGui::Text("Rotation (Euler):");
        if (isAnimated) {
            auto& rotState = rotationEditorState(e);
            const glm::quat currentRotation = glm::normalize(t.rotation);
            if (!rotState.initialized || !sameRotation(rotState.sourceQuat, currentRotation)) {
                rotState.sourceQuat = currentRotation;
                rotState.eulerDeg = glm::degrees(glm::eulerAngles(currentRotation));
                rotState.initialized = true;
            }
            if (ImGui::DragFloat3("##Rotation", glm::value_ptr(rotState.eulerDeg), 0.1f)) {
                const glm::quat edited = glm::normalize(glm::quat(glm::radians(rotState.eulerDeg)));
                t.setRotation(edited);
                rotState.sourceQuat = edited;
                update = true;
            }
        } else {
            glm::vec3 euler = glm::degrees(glm::eulerAngles(t.rotation));
            if (ImGui::DragFloat3("##Rotation", glm::value_ptr(euler), 0.1f)) {
                t.setRotation(glm::quat(glm::radians(euler)));
                update = true;
            }
        }

        if (isAnimated) {
            auto& anim = r.get<ecs::TransformAnim>(e);
            keyframeButton(
                r, e, anim, TransformKeyframeType::Scale,
                [&](const bool& hasKeyframe) -> void {
                    if (!hasKeyframe) anim.insertScaleKeyframe(Core::getAnimation().getFrame(), t.scale);
                    else anim.removeScaleKeyframe(Core::getAnimation().getFrame());
                }
            );
        }
        ImGui::Text("Scale:");
        if (ImGui::DragFloat3("##Scale", glm::value_ptr(t.scale), 0.01f)) {
            t.scale = glm::max(glm::vec3(1e-6f), t.scale);
            t.updated = true;
            update = true;
        }

        ImGui::PopItemWidth();

        return update;
    });

    ui_reg.add<ecs::TransformAnim>([](ecs::TransformAnim& p, ecs::Registry& r, ecs::Entity e){
        ImGui::CollapsingHeader(ICON_FA_ARROWS_UP_DOWN_LEFT_RIGHT " Transform Anim", ImGuiTreeNodeFlags_Bullet);
        return false;
    });

    ui_reg.add<ecs::MaterialAnim>([](ecs::MaterialAnim& anim, ecs::Registry& r, ecs::Entity e){
        ImGui::CollapsingHeader(ICON_FA_PALETTE " Material Anim", ImGuiTreeNodeFlags_Bullet);

        auto* mats = ComponentUiRegistry::get().materials;
        auto& matRefs = r.storage<ecs::MaterialRef>();
        if (!mats || mats->empty() || !matRefs.has(e)) return false;

        const Material& mat = (*mats)[matRefs.get(e).handle];
        const int frame = Core::getAnimation().getFrame();
        const bool hasKeyframe = anim.hasKeyframe(frame);

        if (hasKeyframe) ImGui::PushStyleColor(ImGuiCol_Text, ui::kKeyframeOnColor);
        else             ImGui::PushStyleColor(ImGuiCol_Text, ui::kKeyframeOffColor);

        ImGui::PushID((long long)&anim);
        ui::PushTransparentStyleColor();
        const char* label = hasKeyframe ? ICON_FA_SQUARE " Remove keyframe" : ICON_FA_SQUARE " Insert keyframe";
        if (ImGui::Button(label)) {
            if (hasKeyframe) anim.removeKeyframe(frame);
            else             anim.insertKeyframe(frame, mat.roughness, mat.metalness, mat.ior, mat.transmission, mat.emissionStrength);
        }
        ui::PopTransparentStyleColor();
        ImGui::PopID();

        ImGui::PopStyleColor();
        return false;
    });

    ui_reg.add<ecs::MaterialRef>([](ecs::MaterialRef& ref, ecs::Registry& r, ecs::Entity e){
        auto* mats = ComponentUiRegistry::get().materials;
        if (!mats || mats->empty()) return false;

        if (!ImGui::CollapsingHeader(ICON_FA_PALETTE " Material")) return false;
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
