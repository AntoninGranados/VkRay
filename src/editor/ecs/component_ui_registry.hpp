#pragma once

#include <functional>
#include <vector>

#include "imgui/imgui.h"

#include "core/ecs/components/component_type.hpp"
#include "core/ecs/registry.hpp"
#include "editor/ui_utils.hpp"

namespace ecs {

class ComponentUiRegistry {
public:
    using Drawer = std::function<void(Registry&, Entity)>;

    void add(const ComponentType& componentType);
    void add(const ComponentType& type, std::function<bool(Component&, Registry&, Entity)> extra);
    void addCustom(const ComponentType& type, std::function<bool(Component&, Registry&, Entity)> custom);

    void draw(Registry& registry, Entity e) const {
        for (const Drawer& drawer : drawers) drawer(registry, e);
        registry.flush();
    }

    static ComponentUiRegistry& get();
    static void init();

private:
    std::vector<Drawer> drawers;

    void addWithFields(const ComponentType& type, std::function<bool(Component&, Registry&, Entity)> extra,
                       bool bulletIfEmpty);

    static bool beginDraw(void* id, const ComponentType& type) {
        ImGui::PushID(id);
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0, 0, 0, 0.2));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0, 0, 0, 0));
        ImGui::BeginChild("Component", ImVec2{0, 0}, ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY,
                          ImGuiWindowFlags_None);

        ImGui::BeginDisabled(type.isPermanent());
        bool remove = ui::minusButton("Remove");
        ImGui::EndDisabled();
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
