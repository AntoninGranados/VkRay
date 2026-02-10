#include "./component_ui_registry.hpp"

#include <limits>
#include <cmath>
#include <unordered_map>

#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>

#include "IconsFontAwesome7.h"

#include "../scene/asset/mesh.hpp"
#include "../animation_handler.hpp"
#include "../ui_constants.hpp"

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


void keyframeButton(AppContext& ctx, ecs::Registry& r, ecs::Entity& e, ecs::TransformAnim& anim, const TransformKeyframeType& type, std::function<void(const bool&)> func) {
    bool hasKeyframe;
    switch (type) {
        case TransformKeyframeType::Position:   hasKeyframe = anim.hasPositionKeyframe(ctx.animation->getFrame());  break;
        case TransformKeyframeType::Rotation:   hasKeyframe = anim.hasRotationKeyframe(ctx.animation->getFrame());  break;
        case TransformKeyframeType::Scale:      hasKeyframe = anim.hasScaleKeyframe(ctx.animation->getFrame());     break;
    }
    
    if (hasKeyframe) ImGui::PushStyleColor(ImGuiCol_Text, ui::keyframe_on_col);
    else ImGui::PushStyleColor(ImGuiCol_Text, ui::keyframe_off_col);
    
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

    ui_reg.add<ecs::Name>([](ecs::Name& n, AppContext& ctx, ecs::Registry& r, ecs::Entity e){
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

    ui_reg.add<ecs::Sphere>([](ecs::Sphere& s, AppContext& ctx, ecs::Registry& r, ecs::Entity e){
        bool update = false;
        
        if (!ImGui::CollapsingHeader(ICON_FA_CIRCLE " Sphere")) return false;

        ImGui::PushItemWidth(-FLT_MIN);
        ImGui::Text("Radius:");
        update |= ImGui::DragFloat("##Radius", &s.radius, 0.01f, 0.0f, FLT_MAX);
        ImGui::PopItemWidth();

        return update;
    });
    
    ui_reg.add<ecs::Plane>([](ecs::Plane& p, AppContext& ctx, ecs::Registry& r, ecs::Entity e){
        ImGui::CollapsingHeader(ICON_FA_SQUARE " Plane", ImGuiTreeNodeFlags_Bullet);
        return false;
    });
    
    ui_reg.add<ecs::Box>([](ecs::Box& p, AppContext& ctx, ecs::Registry& r, ecs::Entity e){
        ImGui::CollapsingHeader(ICON_FA_BOX " Box", ImGuiTreeNodeFlags_Bullet);
        return false;
    });
    
    ui_reg.add<ecs::MeshRef>([](ecs::MeshRef& ref, AppContext& ctx, ecs::Registry& r, ecs::Entity e){
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

    ui_reg.add<ecs::CameraObject>([](ecs::CameraObject& c, AppContext& ctx, ecs::Registry& r, ecs::Entity e){
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

    ui_reg.add<ecs::Transform>([](ecs::Transform& t, AppContext& ctx, ecs::Registry& r, ecs::Entity e){
        bool update = false;

        if (!ImGui::CollapsingHeader(ICON_FA_ARROWS_UP_DOWN_LEFT_RIGHT " Transform")) return false;

        bool isAnimated = r.has<ecs::TransformAnim>(e);

        ImGui::PushItemWidth(-FLT_MIN);

        if (isAnimated) {
            auto& anim = r.get<ecs::TransformAnim>(e);
            keyframeButton(
                ctx, r, e, anim, TransformKeyframeType::Position,
                [&](const bool& hasKeyframe) -> void {
                    if (!hasKeyframe) anim.insertPositionKeyframe(ctx.animation->getFrame(), t.position);
                    else anim.removePositionKeyframe(ctx.animation->getFrame());
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
                ctx, r, e, anim, TransformKeyframeType::Rotation,
                [&](const bool& hasKeyframe) -> void {
                    if (!hasKeyframe) anim.insertRotationKeyframe(ctx.animation->getFrame(), t.rotation);
                    else anim.removeRotationKeyframe(ctx.animation->getFrame());
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
                ctx, r, e, anim, TransformKeyframeType::Scale,
                [&](const bool& hasKeyframe) -> void {
                    if (!hasKeyframe) anim.insertScaleKeyframe(ctx.animation->getFrame(), t.scale);
                    else anim.removeScaleKeyframe(ctx.animation->getFrame());
                }
            );
        }
        ImGui::Text("Scale:");
        if (ImGui::DragFloat3("##Scale", glm::value_ptr(t.scale), 0.01f)) {
            t.updated = true;
            update = true;
        }
        
        ImGui::PopItemWidth();

        return update;
    });

    ui_reg.add<ecs::TransformAnim>([](ecs::TransformAnim& p, AppContext& ctx, ecs::Registry& r, ecs::Entity e){
        ImGui::CollapsingHeader(ICON_FA_ARROWS_UP_DOWN_LEFT_RIGHT " Transform Anim", ImGuiTreeNodeFlags_Bullet);
        return false;
    });

    ui_reg.add<ecs::MaterialRef>([](ecs::MaterialRef& ref, AppContext& ctx, ecs::Registry& r, ecs::Entity e){
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
