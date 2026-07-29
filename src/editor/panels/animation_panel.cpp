#include "animation_panel.hpp"

#include "imgui/imgui.h"
#include "FontAwesome/IconsFontAwesome7.h"

#include "core/animation_handler.hpp"
#include "core/core.hpp"
#include "core/scene/scene.hpp"
#include "editor/ui_constants.hpp"

void AnimationPanel::content() {
    Scene& scene = Core::getScene();

    ui::setNextWindowFixed();
    ImGui::Begin("Animation");
    {
        bool paused = Core::getAnimation().isPaused();
        if (ImGui::Button((paused ? ICON_FA_PLAY : ICON_FA_PAUSE), { 20, 0 })) {
            Core::getAnimation().toggle();
        }
        ImGui::SameLine();
        const bool bakingPhysics = scene.isPhysicsBakeInProgress();
        if (bakingPhysics) {
            int totalBakeFrames = std::max(1, scene.getPhysicsBakeTotalFrames());
            int currentBakeFrame = std::clamp(scene.getPhysicsBakeCurrentFrame(), 0, totalBakeFrames);
            float bakeProgress = static_cast<float>(currentBakeFrame) / static_cast<float>(totalBakeFrames);
            char overlay[16];
            std::snprintf(overlay, sizeof(overlay), "%.0f%%", bakeProgress * 100.0f);

            ImGui::ProgressBar(bakeProgress, ImVec2(120.0f, 0.0f), "");

            ImVec2 textSize = ImGui::CalcTextSize(overlay);
            ImVec2 barMin = ImGui::GetItemRectMin();
            ImVec2 barMax = ImGui::GetItemRectMax();
            ImVec2 textPos(
                (barMin.x + barMax.x - textSize.x) * 0.5f,
                (barMin.y + barMax.y - textSize.y) * 0.5f
            );
            ImGui::GetWindowDrawList()->AddText(textPos, ImGui::GetColorU32(ImGuiCol_Text), overlay);
        } else if (ImGui::Button(ICON_FA_HARD_DRIVE " Bake Physics", { 120, 0 })) {
            scene.bakePhysics();
        }
        ImGui::SameLine();

        // TODO: put this in the constants
        const float barWidth = 400.0f;
        const float barHeight = 16.0f;

        ImDrawList* dl = ImGui::GetWindowDrawList();
        const float frameHeight = ImGui::GetFrameHeight();
        ImVec2 p = ImGui::GetCursorScreenPos();

        // TODO: make this capture the mouse so we can drag even when going outside of the bar
        ImGui::InvisibleButton("Timeline", ImVec2(barWidth, frameHeight));
        bool hovered = ImGui::IsItemHovered();
        if (hovered && ImGui::IsMouseDown(0)) {
            float t = (ImGui::GetIO().MousePos.x - p.x) / barWidth;
            t = std::clamp(t, 0.0f, 1.0f);
            Core::getAnimation().reset(static_cast<int>(t * Core::getAnimation().getEndFrame()));
            Core::getAnimation().pause();
        }

        // Draw bar
        ImU32 barCol = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[ImGuiCol_FrameBg]);
        ImU32 fillCol = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[ImGuiCol_SliderGrab]);
        ImVec2 barMin = ImVec2(p.x, p.y + (frameHeight - barHeight) * 0.5f);
        ImVec2 barMax = ImVec2(p.x + barWidth, barMin.y + barHeight);
        dl->AddRectFilled(barMin, barMax, barCol, 3.0f);

        // Draw current time fill
        float tNorm = (Core::getAnimation().getFrame() > 0.0f) ? (float(Core::getAnimation().getFrame()) / Core::getAnimation().getEndFrame()) : 0.0f;
        tNorm = std::clamp(tNorm, 0.0f, 1.0f);
        dl->AddRectFilled(ImVec2(barMin.x + barWidth * tNorm - 4.0f, barMin.y), ImVec2(barMin.x + barWidth * tNorm + 4.0f, barMax.y), fillCol, 3.0f);
        
        // Draw keyframes
        /* TODO: rewire to new storage
        const SceneSelection& sel = Editor::getUi().getSelection();
        if (sel.entity >= 0) {
            const ecs::Entity e = Core::getScene().getEntities()[static_cast<size_t>(sel.entity)];
            if (Core::getScene().getRegistry().has<ecs::TransformAnim>(e)) {
                auto& anim = Core::getScene().getRegistry().get<ecs::TransformAnim>(e);
                for (auto& k : anim.positionKeys) {
                    float x = barMin.x + (float(k.frame) / Core::getAnimation().getEndFrame()) * barWidth;
                    ImVec2 c = ImVec2(x, barMin.y + barHeight * 0.5f);
                    dl->AddCircleFilled(c, ui::kWidgetRounding*1.5f, ImGui::ColorConvertFloat4ToU32(ui::kKeyframeOffColor));
                    dl->AddCircleFilled(c, ui::kWidgetRounding, ImGui::ColorConvertFloat4ToU32(ui::kKeyframeOnColor));
                }
            }
        }
        */

        // Draw current frame
        std::string frameLabel = std::to_string(Core::getAnimation().getFrame());
        ImVec2 frameLabelSize = ImGui::CalcTextSize(frameLabel.c_str());
        ImVec2 frameLabelPos(
            (barMin.x + barMax.x - frameLabelSize.x) * 0.5f,
            (barMin.y + barMax.y - frameLabelSize.y) * 0.5f
        );
        dl->AddText(frameLabelPos, ImGui::GetColorU32(ImGuiCol_Text), frameLabel.c_str());
        
        ImGui::SameLine();
        
        ImGui::PushItemWidth(40);
        int endFrame = Core::getAnimation().getEndFrame();
        if (ImGui::DragInt("##EndFrame", &endFrame, 1, 1)) {
            Core::getAnimation().pause();
            Core::getAnimation().setEndFrame(endFrame);
        }
        ImGui::PopItemWidth();
    }
    ImGui::End();
}
