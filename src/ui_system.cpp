#include "ui_system.hpp"

#include "notification_system.hpp"

void UiSystem::draw(CommandBuffer commandBuffer, AppContext& ctx) {
    if (!toggled && !ctx.renderState->renderMode) return;

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    
    ImGui::NewFrame();
    updateState();

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;
    style.FrameRounding = 5.0f;

    if (ctx.renderState->renderMode) drawRender(ctx);
    else drawPreview(ctx);

    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer.get());

}

void UiSystem::updateState() {
    ImGuiIO& io = ImGui::GetIO();
    capturesMouse = io.WantCaptureMouse;
    capturesKeyboard = io.WantCaptureKeyboard;
}

void UiSystem::drawPreview(AppContext& ctx) {
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

        scene.runDrawSystems(ctx);

        scene.drawGuizmo(
            camera.getView(),
            camera.getProjection(ctx.engine->getWindow().get())
        );
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowBgAlpha(0.8f);
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

    ImGui::SetNextWindowBgAlpha(0.8f);
    ImGui::Begin("Information", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    {
        if (ImGui::CollapsingHeader("Camera")) {
            camera.drawPreviewUI(restartRender);
        }
        
        if (ImGui::CollapsingHeader("Pathtracer")) {
            ctx.parameters->drawGroup("Pathtracer", restartRender);
        }
        
        if (ImGui::CollapsingHeader("Scene")) {
            ctx.parameters->drawGroup("Scene", restartRender);
            if (ImGui::Button("Load Scene Preset", { -FLT_MIN, 0 }) && !ImGui::IsPopupOpen("Scene Preset")) {
                ImGui::OpenPopup("Scene Preset");
            }
            scene.drawUI();
        }

        if (ImGui::BeginPopupModal("Scene Preset", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
            LightMode mode = ctx.parameters->getEnum<LightMode>("lightMode");
            if (ImGui::Button("Empty", { 200, 0 })) {
                mode = scene.loadPreset(ScenePreset::Empty);
                restartRender = true;
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::Button("Mesh", { 200, 0 })) {
                mode = scene.loadPreset(ScenePreset::Mesh);
                restartRender = true;
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::Button("Sponza", { 200, 0 })) {
                mode = scene.loadPreset(ScenePreset::Sponza);
                restartRender = true;
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::Button("Cornell Box", { 200, 0 })) {
                mode = scene.loadPreset(ScenePreset::CornellBox);
                restartRender = true;
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::Button("Random Spheres", { 200, 0 })) {
                mode = scene.loadPreset(ScenePreset::RandomSpheres);
                restartRender = true;
                ImGui::CloseCurrentPopup();
            }
            ctx.parameters->setEnum<LightMode>("lightMode", mode);
            
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.15f, 0.15f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.25f, 0.25f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));
            if (ImGui::Button("Cancel", { 200, 0 })) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopStyleColor(3);
            
            ImGui::EndPopup();
        }
    }
    ImGui::End();
    
    scene.drawSelectedEntityUI();
    scene.drawSelectedMaterialUI();
    scene.drawSelectedMeshAssetUI();
    ctx.notifications->drawNotifications();
}

void UiSystem::drawRender(AppContext& ctx) {
    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowBgAlpha(0.8f);
    ImGui::Begin("Loading",
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
