#include "scene_panel.hpp"

#include <format>

#include "imgui/imgui.h"
#include "FontAwesome/IconsFontAwesome7.h"
#include <nfd.hpp>

#include "app/app_context.hpp"
#include "app/log.hpp"
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
            if (NFD::OpenDialog(outPath, filter, 1, "res/scenes/") == NFD_OKAY) {
                LightMode mode = ctx.parameters->getEnum<LightMode>("scene/light_mode");
                if (SceneSerializer::load(*ctx.scene, mode, outPath.get())) {
                    ctx.parameters->setEnum<LightMode>("scene/light_mode", mode);
                    *ctx.restartRender = true;
                    Log::success("ScenePanel", std::format("Scene loaded: {}", outPath.get()));
                }
            }
        }

        if (ImGui::Button(ICON_FA_FLOPPY_DISK " Save Scene", { -FLT_MIN, 0 })) {
            NFD::Guard guard;
            NFD::UniquePath outPath;
            nfdfilteritem_t filter[1] = { { "Scene", "json" } };
            if (NFD::SaveDialog(outPath, filter, 1, "res/scenes/", "untitled.json") == NFD_OKAY) {
                std::string path = outPath.get();
                if (path.size() < 5 || path.substr(path.size() - 5) != ".json")
                    path += ".json";
                LightMode mode = ctx.parameters->getEnum<LightMode>("scene/light_mode");
                if (SceneSerializer::save(*ctx.scene, mode, path)) {
                    Log::success("ScenePanel", std::format("Scene saved: {}", path));
                }
            }
        }

        ctx.parameters->drawGroup("scene", *ctx.restartRender);
        ctx.scene->drawUI();
    }
    ImGui::End();
}
