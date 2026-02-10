#include "ui_handler.hpp"

#include "IconsFontAwesome7.h"

#include "./parameter_handler.hpp"
#include "./scene/scene_preset.hpp"
#include "./notification_handler.hpp"
#include "./animation_handler.hpp"

#include "./ui_constants.hpp"


void UiHandler::draw(CommandBuffer commandBuffer, AppContext& ctx) {
    if (!toggled && ctx.renderState->renderMode == RenderMode::Preview) return;

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    
    ImGui::NewFrame();
    updateState();

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = ui::widget_rounding;
    style.FrameRounding = ui::widget_rounding;

    if (ctx.renderState->renderMode != RenderMode::Preview) drawRender(ctx);
    else drawPreview(ctx);

    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer.get());

}

void UiHandler::updateState() {
    ImGuiIO& io = ImGui::GetIO();
    capturesMouse = io.WantCaptureMouse;
    capturesKeyboard = io.WantCaptureKeyboard;
}

void UiHandler::drawPreview(AppContext& ctx) {
    bool& restartRender = *ctx.restartRender;
    Camera& camera = *ctx.camera;
    Scene& scene = *ctx.scene;

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::AllowAxisFlip(false);
    ImGuizmo::BeginFrame();
    
    ImGuiID dockspace_id = ImGui::GetID("Dock space");
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_PassthruCentralNode;
    ImGui::DockSpaceOverViewport(dockspace_id, ImGui::GetMainViewport(), dockspaceFlags);

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("Gizmo View", nullptr, ImGuiWindowFlags_NoBackground | 
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoMouseInputs |
        ImGuiWindowFlags_NoDocking
    );
    {
        ImVec2 windowPos = ImGui::GetWindowPos();
        ImVec2 windowSize = ImGui::GetWindowSize();
        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(windowPos.x, windowPos.y, windowSize.x, windowSize.y);

        scene.runOnUi(ctx);

        scene.drawGuizmo(
            camera.getView(),
            camera.getProjection(ctx.engine->getWindow().get())
        );
    }
    ImGui::End();
    ImGui::PopStyleVar(2);

    ImGui::SetNextWindowPos({ 0, 0 });
    ImGui::SetNextWindowBgAlpha(ui::window_bg_alpha);
    ImGui::Begin("FPS",
        nullptr,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration
    );
    {
        ImGui::Text("%.1f fps (%.3f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
        ImGui::Text("%u samples",ctx.renderState->sampleCount);
        ImGui::Text("%.0f samples/sec", ImGui::GetIO().Framerate * ctx.parameters->getInt("previewSamples"));
    }
    ImGui::End();

    ImGuiViewport* mainViewport = ImGui::GetMainViewport();
    ImVec2 animPos(
        mainViewport->Pos.x + mainViewport->Size.x * 0.5f,
        mainViewport->Pos.y + mainViewport->Size.y - 10.0f
    );
    ImGui::SetNextWindowPos(animPos, ImGuiCond_Always, ImVec2(0.5f, 1.0f));
    ImGui::SetNextWindowBgAlpha(ui::window_bg_alpha);
    ImGui::Begin("Animation",
        nullptr,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration
    );
    {
        bool paused = ctx.animation->isPaused();
        if (ImGui::Button((paused ? ICON_FA_PLAY : ICON_FA_PAUSE), { 20, 0 })) {
            ctx.animation->toggle();
        }
        ImGui::SameLine();

        const float barWidth = 400.0f;
        const float barHeight = 16.0f;

        ImDrawList* dl = ImGui::GetWindowDrawList();
        const float frameHeight = ImGui::GetFrameHeight();
        ImVec2 p = ImGui::GetCursorScreenPos();

        ImGui::InvisibleButton("Timeline", ImVec2(barWidth, frameHeight));
        bool hovered = ImGui::IsItemHovered();
        if (hovered && ImGui::IsMouseDown(0)) {
            float t = (ImGui::GetIO().MousePos.x - p.x) / barWidth;
            t = std::clamp(t, 0.0f, 1.0f);
            ctx.animation->reset(static_cast<int>(t * ctx.animation->getEndFrame()));
            ctx.animation->pause();
        }

        // Draw bar
        ImU32 barCol = IM_COL32(140, 140, 140, 128);
        ImU32 fillCol = IM_COL32(80, 160, 255, 255);
        ImVec2 barMin = ImVec2(p.x, p.y + (frameHeight - barHeight) * 0.5f);
        ImVec2 barMax = ImVec2(p.x + barWidth, barMin.y + barHeight);
        dl->AddRectFilled(barMin, barMax, barCol, 3.0f);

        // Draw current time fill
        float tNorm = (ctx.animation->getFrame() > 0.0f) ? (float(ctx.animation->getFrame()) / ctx.animation->getEndFrame()) : 0.0f;
        tNorm = std::clamp(tNorm, 0.0f, 1.0f);
        dl->AddRectFilled(ImVec2(barMin.x + barWidth * tNorm - 4.0f, barMin.y), ImVec2(barMin.x + barWidth * tNorm + 4.0f, barMax.y), fillCol, 3.0f);
        
        // Draw keyframes
        const ecs::Entity* e = ctx.scene->getSelectedEntity();
        if (e && ctx.scene->getRegistry().has<ecs::TransformAnim>(*e)) {
            auto& anim = ctx.scene->getRegistry().get<ecs::TransformAnim>(*e);
            for (auto& k : anim.positionKeys) {
                float x = barMin.x + (float(k.frame) / ctx.animation->getEndFrame()) * barWidth;
                ImVec2 c = ImVec2(x, barMin.y + barHeight * 0.5f);
                dl->AddCircleFilled(c, ui::widget_rounding*1.5f, ImGui::ColorConvertFloat4ToU32(ui::keyframe_off_col));
                dl->AddCircleFilled(c, ui::widget_rounding, ImGui::ColorConvertFloat4ToU32(ui::keyframe_on_col));
            }
        }
        
        ImGui::SameLine();
        
        ImGui::PushItemWidth(40);
        int endFrame = ctx.animation->getEndFrame();
        if (ImGui::DragInt("##EndFrame", &endFrame, 1, 1, 500)) {
            ctx.animation->pause();
            ctx.animation->setEndFrame(endFrame);
        }
        ImGui::PopItemWidth();
    }
    ImGui::End();

    ImGui::SetNextWindowBgAlpha(ui::window_bg_alpha);
    ImGui::Begin(ICON_FA_CIRCLE_INFO " Information", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    {
        if (ImGui::CollapsingHeader(ICON_FA_VIDEO " Camera")) {
            camera.drawPreviewUI(restartRender);
        }
        
        if (ImGui::CollapsingHeader(ICON_FA_GEAR " Pathtracer")) {
            ctx.parameters->drawGroup("Pathtracer", restartRender);
        }
        
        if (ImGui::CollapsingHeader(ICON_FA_CUBES " Scene")) {
            ctx.parameters->drawGroup("Scene", restartRender);
            if (ImGui::Button(ICON_FA_LIST " Load Scene Preset", { -FLT_MIN, 0 }) && !ImGui::IsPopupOpen("Scene Preset")) {
                ImGui::OpenPopup(ICON_FA_LIST " Scene Preset");
            }
            scene.drawUI();
        }

        if (ImGui::BeginPopupModal(ICON_FA_LIST " Scene Preset", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
            LightMode mode = ctx.parameters->getEnum<LightMode>("lightMode");
            for (auto& [preset, name] : scenePresetName) {
                if (ImGui::Button((name + "##ScenePreset").c_str(), ui::button_size)) {
                    scenePresetInitMethod[preset](*ctx.scene, mode);
                    restartRender = true;
                    ImGui::CloseCurrentPopup();
                }
            }
            ctx.parameters->setEnum<LightMode>("lightMode", mode);
            
            ui::PushCancelStyleColor();
            if (ImGui::Button(ICON_FA_BAN " Cancel", ui::button_size)) {
                ImGui::CloseCurrentPopup();
            }
            ui::PopCancelStyleColor();
            
            ImGui::EndPopup();
        }
    }
    ImGui::End();
    
    scene.drawSelectedEntityUI();
    scene.drawSelectedMaterialUI();
    scene.drawSelectedMeshAssetUI();
    ctx.notifications->drawNotifications();
}

void UiHandler::drawRender(AppContext& ctx) {
    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowBgAlpha(ui::window_bg_alpha);
    ImGui::Begin(ICON_FA_STOPWATCH " Loading",
        nullptr,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration
    );
    int renderSamplesPerPixel = ctx.parameters->getInt("renderSamples");
    if (renderSamplesPerPixel > 0) {
        float progress = static_cast<float>(std::min<uint64_t>(ctx.renderState->sampleCount, renderSamplesPerPixel))
            / static_cast<float>(renderSamplesPerPixel);
        char overlay[64];
        snprintf(
            overlay,
            sizeof(overlay),
            "%llu / %d",
            static_cast<unsigned long long>(std::min<uint64_t>(ctx.renderState->sampleCount, renderSamplesPerPixel)),
            renderSamplesPerPixel
        );
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.55f, 0.55f, 0.55f, 0.85f));
        ImGui::ProgressBar(progress, ImVec2(ImGui::GetContentRegionAvail().x, 0.0f), "");
        ImGui::PopStyleColor();

        ImVec2 textSize = ImGui::CalcTextSize(overlay);
        ImVec2 barMin = ImGui::GetItemRectMin();
        ImVec2 barMax = ImGui::GetItemRectMax();
        ImVec2 textPos(
            (barMin.x + barMax.x - textSize.x) * 0.5f,
            (barMin.y + barMax.y - textSize.y) * 0.5f
        );
        ImGui::GetWindowDrawList()->AddText(textPos, ImGui::GetColorU32(ImGuiCol_Text), overlay);
    }
    float samplesPerSec = static_cast<float>(ctx.renderState->samplesPerSecEMA);
    ImGui::Text("%.1f samples/sec", samplesPerSec);
    if (renderSamplesPerPixel > 0 && samplesPerSec > 0.0f) {
        uint64_t remaining = 0;
        if (ctx.renderState->sampleCount < static_cast<uint64_t>(renderSamplesPerPixel)) {
            remaining = static_cast<uint64_t>(renderSamplesPerPixel) - ctx.renderState->sampleCount;
        }
        float etaSec = static_cast<float>(remaining) / samplesPerSec;
        int etaMin = static_cast<int>(etaSec / 60.0f);
        int etaRemSec = static_cast<int>(etaSec) % 60;
        ImGui::Text("ETA: %dm %02ds", etaMin, etaRemSec);
    } else {
        ImGui::Text("ETA: --");
    }
    ImGui::End();
}
