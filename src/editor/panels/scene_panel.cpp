#include "scene_panel.hpp"

#include "imgui/imgui.h"
#include "FontAwesome/IconsFontAwesome7.h"
#include <nfd.hpp>

#include "app/app_context.hpp"
#include "app/notification_handler.hpp"
#include "app/parameter_handler.hpp"
#include "editor/ui_constants.hpp"

#include "scene/scene.hpp"
#include "scene/scene_serializer.hpp"


void ScenePanel::draw(AppContext& ctx) {
    ImGui::SetNextWindowBgAlpha(ui::kWindowBgAlpha);
    ImGui::Begin(ICON_FA_CUBES " Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    {
        if (ImGui::Button(ICON_FA_UPLOAD " Load Scene", { -FLT_MIN, 0 })) {
            NFD::Guard guard;
            NFD::UniquePath outPath;
            nfdfilteritem_t filter[1] = { { "Scene", "json" } };
            if (NFD::OpenDialog(outPath, filter, 1, "scenes/") == NFD_OKAY) {
                LightMode mode = ctx.parameters->getEnum<LightMode>("lightMode");
                if (SceneSerializer::load(*ctx.scene, mode, outPath.get())) {
                    ctx.parameters->setEnum<LightMode>("lightMode", mode);
                    *ctx.restartRender = true;
                    ctx.notifications->pushMessage(NotificationType::Info, "Scene loaded: " + std::string(outPath.get()));
                } else {
                    ctx.notifications->pushMessage(NotificationType::Error, "Failed to load scene: " + std::string(outPath.get()));
                }
            }
        }

        if (ImGui::Button(ICON_FA_FLOPPY_DISK " Save Scene", { -FLT_MIN, 0 })) {
            NFD::Guard guard;
            NFD::UniquePath outPath;
            nfdfilteritem_t filter[1] = { { "Scene", "json" } };
            if (NFD::SaveDialog(outPath, filter, 1, "scenes/", "untitled.json") == NFD_OKAY) {
                std::string path = outPath.get();
                if (path.size() < 5 || path.substr(path.size() - 5) != ".json")
                    path += ".json";
                LightMode mode = ctx.parameters->getEnum<LightMode>("lightMode");
                if (SceneSerializer::save(*ctx.scene, mode, path)) {
                    ctx.notifications->pushMessage(NotificationType::Info, "Scene saved: " + path);
                } else {
                    ctx.notifications->pushMessage(NotificationType::Error, "Failed to save scene: " + path);
                }
            }
        }

        ctx.parameters->drawGroup("Scene", *ctx.restartRender);
        ctx.scene->drawUI();
    }
    ImGui::End();
}
