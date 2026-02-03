#pragma once

#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "./registry.hpp"
#include "./components.hpp"
#include "../app_context.hpp"

#include "./imgui/imgui.h"

class MeshAsset;

namespace ecs {

class ComponentUiRegistry {
public:
    using Drawer = std::function<bool(AppContext& ctx, Registry&, Entity)>;

    void addDrawer(Drawer drawer) {
        drawers.push_back(std::move(drawer));
    }

    template<typename T>
    void add(std::function<bool(T& t, AppContext& ctx, Registry& registry, Entity e)> func) {
        drawers.emplace_back([func](AppContext& ctx, Registry& registry, Entity e) {
            if (!registry.has<T>(e)) return false;

            T& t = registry.get<T>(e);
            ImGui::PushID(&t);
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0,0,0,0));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0,0,0,0.2));
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0,0,0,0));
            ImGui::BeginChild("Component", ImVec2{0, 0}, ImGuiChildFlags_FrameStyle | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_None);
            
            if (ImGui::Button("-##Remove", { 32, 0 })) {
                registry.remove<T>(e);
                *ctx.restartRender = true;
            }
            ImGui::SameLine();
            bool update = func(t, ctx, registry, e);
            
            ImGui::EndChild();
            ImGui::PopStyleColor(3);
            ImGui::PopID();
            return update;
        });
    }

    bool draw(AppContext& ctx, Registry& registry, Entity e) const {
        bool changed = false;
        for (const Drawer& drawer : drawers)
            changed |= drawer(ctx, registry, e);
        return changed;
    }

    static ComponentUiRegistry& get();
    static void init();
    void setMaterials(std::vector<Material>* materials_) { materials = materials_; }
    void setMeshAssets(std::vector<MeshAsset>* meshAssets_) { meshAssets = meshAssets_; }

private:
    std::vector<Drawer> drawers;
    std::vector<Material>* materials = nullptr;
    std::vector<MeshAsset>* meshAssets = nullptr;
};

} // namespace ecs
