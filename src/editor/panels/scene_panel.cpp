#include "scene_panel.hpp"

#include "imgui/imgui.h"
#include "IconsFontAwesome7.h"
#include "editor/ui_constants.hpp"

#include "app/app_context.hpp"
#include "app/parameter_handler.hpp"

#include "scene/scene.hpp"
#include "scene/scene_preset.hpp"

void ScenePanel::draw(AppContext& ctx) {
    bool openScenePresetSelection = false;

    ImGui::SetNextWindowBgAlpha(ui::kWindowBgAlpha);
    ImGui::Begin(ICON_FA_CUBES " Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    {
        ctx.parameters->drawGroup("Scene", *ctx.restartRender);
        if (ImGui::Button(ICON_FA_LIST " Load Scene Preset", { -FLT_MIN, 0 }) && !ImGui::IsPopupOpen("Scene Preset")) {
            openScenePresetSelection = true;
        }
        ctx.scene->drawUI();
    }
    ImGui::End();

    if (openScenePresetSelection) ImGui::OpenPopup(ICON_FA_LIST " Scene Preset");
    if (ImGui::BeginPopupModal(ICON_FA_LIST " Scene Preset", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        LightMode mode = ctx.parameters->getEnum<LightMode>("lightMode");
        for (auto& [preset, name] : scenePresetName) {
            if (ImGui::Button((name + "##ScenePreset").c_str(), ui::kButtonSize)) {
                scenePresetInitMethod[preset](*ctx.scene, mode);
                *ctx.restartRender = true;
                ImGui::CloseCurrentPopup();
            }
        }
        ctx.parameters->setEnum<LightMode>("lightMode", mode);
        
        ui::PushCancelStyleColor();
        if (ImGui::Button(ICON_FA_BAN " Cancel", ui::kButtonSize)) {
            ImGui::CloseCurrentPopup();
        }
        ui::PopCancelStyleColor();
        
        ImGui::EndPopup();
    }
}
