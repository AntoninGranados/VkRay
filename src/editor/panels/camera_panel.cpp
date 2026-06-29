#include "camera_panel.hpp"

#include "imgui/imgui.h"
#include "FontAwesome/IconsFontAwesome7.h"

#include "app/app_context.hpp"
#include "editor/ui_constants.hpp"
#include "camera.hpp"

void CameraPanel::draw(AppContext& ctx) {
    Camera& camera = *ctx.camera;

    ImGui::SetNextWindowBgAlpha(ui::kWindowBgAlpha);
    ImGui::Begin(ICON_FA_VIDEO " Camera", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    {
         bool updated = false;

        glm::vec3 dir = camera.getDirection();
        ImGui::Text("Camera Position :\n (%4.1f, %4.1f, %4.1f)", camera.position.x, camera.position.y, camera.position.z);
        ImGui::Text("Camera Direction:\n (%4.1f, %4.1f, %4.1f)", dir.x, dir.y, dir.z);
        ImGui::Text("Camera Target   :\n (%4.1f, %4.1f, %4.1f)", camera.target.x, camera.target.y, camera.target.z);
        ImGui::Text("Camera Fov:\n %4.1f°", camera.getFov());
        
        ImGui::Text("Camera Aperture:");
        ImGui::SetNextItemWidth(-FLT_MIN);
        float newAperture = camera.getAperture();
        if (ImGui::DragFloat("##Camera Aperture", &newAperture, 0.01, 0.0, 5.0)) {
            camera.setAperture(newAperture);
            updated = true;
        }

        ImGui::Text("Camera Focus Depth:");
        ImGui::SetNextItemWidth(-FLT_MIN);
        float newFocusDepth = camera.getFocusDepth();
        if (ImGui::DragFloat("##Camera Focus Depth", &newFocusDepth, 0.1, 0.0, 100.0)) {
            camera.setFocusDepth(newFocusDepth);
            updated = true;
        }

        if (updated) *ctx.restartRender = true;
    }
    ImGui::End();
}
