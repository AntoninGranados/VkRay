#pragma once

#include "./registry.hpp"
#include "./components.hpp"

#include "./imgui/imgui.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

class MeshAsset;

namespace ecs {

class ComponentUiRegistry {
public:
    using Drawer = std::function<bool(Registry&, Entity)>;

    void addDrawer(Drawer drawer) {
        drawers.push_back(std::move(drawer));
    }

    template<typename T, typename Func>
    void add(Func&& func) {
        drawers.emplace_back([fn = std::forward<Func>(func)](Registry& registry, Entity e) {
            if (!registry.has<T>(e)) return false;

            T& t = registry.get<T>(e);
            ImGui::PushID(&t);
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0,0,0,0));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0,0,0,0.2));
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0,0,0,0));
            ImGui::BeginChild("Component", ImVec2{0, 0}, ImGuiChildFlags_FrameStyle | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_None);
            bool update = fn(t, registry, e);
            ImGui::EndChild();
            ImGui::PopStyleColor(3);
            ImGui::PopID();
            return update;
        });
    }

    bool draw(Registry& registry, Entity e) const {
        bool changed = false;
        for (const Drawer& drawer : drawers)
            changed |= drawer(registry, e);
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
