#pragma once

#include <functional>
#include <vector>

#include "imgui/imgui.h"

#include "core/ecs/components/component_type.hpp"
#include "core/ecs/registry.hpp"
#include "core/scene/object/material.hpp"

class MeshAsset;

namespace ecs {

class ComponentUiRegistry {
public:
    using Drawer = std::function<bool(Registry&, Entity)>;

    void add(const ComponentType& componentType);

    void add(const ComponentType& type, std::function<bool(Component&, Registry&, Entity)> func) {
        drawers.emplace_back([func, type](Registry& registry, Entity e) {
            if (!registry.has(e, type)) return false;

            Component& component = registry.get(e, type);
            bool remove = ComponentUiRegistry::beginDraw(&component);
            if (remove) registry.remove(e, type);
            bool update = !remove && func(component, registry, e);
            ComponentUiRegistry::endDraw();
            return remove || update;
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

    static bool drawField(Component& component, const ComponentField& schema);

    static bool beginDraw(void* id) {
        ImGui::PushID(id);
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0,0,0,0.2));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_Border, ImGui::GetStyleColorVec4(ImGuiCol_Separator));
        ImGui::BeginChild("Component", ImVec2{0, 0}, ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_None);

        bool remove = ImGui::Button("-##Remove", { 32, 0 });
        ImGui::SameLine();
        return remove;
    }

    static void endDraw() {
        ImGui::EndChild();
        ImGui::PopStyleColor(4);
        ImGui::PopID();
    }
};

} // namespace ecs
