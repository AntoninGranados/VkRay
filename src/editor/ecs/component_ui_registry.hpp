#pragma once

#include <functional>
#include <vector>

#include "imgui/imgui.h"

#include "core/ecs/components/component_type.hpp"
#include "core/ecs/registry.hpp"

namespace ecs {

class ComponentUiRegistry {
public:
    using Drawer = std::function<bool(Registry&, Entity)>;

    void add(const ComponentType& componentType);
    void add(const ComponentType& type, std::function<bool(Component&, Registry&, Entity)> extra);
    void addCustom(const ComponentType& type, std::function<bool(Component&, Registry&, Entity)> custom);

    bool draw(Registry& registry, Entity e) const {
        bool changed = false;
        for (const Drawer& drawer : drawers)
            changed |= drawer(registry, e);
        return changed;
    }

    static ComponentUiRegistry& get();
    static void init();

private:
    std::vector<Drawer> drawers;

    void addWithFields(const ComponentType& type, std::function<bool(Component&, Registry&, Entity)> extra, bool bulletIfEmpty);

    static bool drawField(Component& component, const ComponentField& schema);

    static bool beginDraw(void* id) {
        ImGui::PushID(id);
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0,0,0,0.2));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0,0,0,0));
        ImGui::BeginChild("Component", ImVec2{0, 0}, ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_None);

        bool remove = ImGui::Button("-##Remove", { 32, 0 });
        ImGui::SameLine();
        return remove;
    }

    static void endDraw() {
        ImGui::EndChild();
        ImGui::PopStyleColor(3);
        ImGui::PopID();
    }
};

} // namespace ecs
