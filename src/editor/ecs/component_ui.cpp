#include "component_ui.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <functional>
#include <memory>
#include <string_view>
#include <utility>

#include "imgui/imgui.h"

#include "core/ecs/components.hpp"
#include "core/render/script_table.hpp"
#include "editor/ecs/component_inspectors.hpp"
#include "editor/fields/field_ui.hpp"
#include "editor/ui_utils.hpp"

namespace ecs {

std::unordered_map<std::string, std::unique_ptr<ComponentInspector>>& ComponentUi::inspectors() {
    static std::unordered_map<std::string, std::unique_ptr<ComponentInspector>> registered;
    return registered;
}

void ComponentUi::registerInspector(const ComponentType& type, std::unique_ptr<ComponentInspector> inspector) {
    inspectors()[type.getId()] = std::move(inspector);
}

ComponentInspector* ComponentUi::findInspector(const ComponentType& type) {
    const auto found = inspectors().find(type.getId());
    return found == inspectors().end() ? nullptr : found->second.get();
}

const std::vector<const ComponentType*>& ComponentUi::drawOrder() {
    static const std::vector<const ComponentType*> order = buildDrawOrder();
    return order;
}

std::ptrdiff_t ComponentUi::groupRank(const ComponentType* type) {
    static constexpr std::array<std::string_view, 10> kGroupOrder = {
        "other",  "movement", "object",      "asset",    "physics",
        "camera", "material", "environment", "internal", "compositing",
    };
    return std::ranges::find(kGroupOrder, type->getGroup()) - kGroupOrder.begin();
}

std::vector<const ComponentType*> ComponentUi::buildDrawOrder() {
    registerBuiltinInspectors();

    std::vector<const ComponentType*> types;
    for (const ComponentType* type : ComponentType::all())
        if (type->getGroup() != "internal" || !type->getFields().empty() || findInspector(*type)) types.push_back(type);

    std::ranges::stable_sort(types, std::less{}, groupRank);
    return types;
}

void ComponentUi::registerBuiltinInspectors() {
    for (const ScriptKind* kind : ScriptTable::componentKinds())
        for (const ComponentType* type : kind->components)
            registerInspector(*type, std::make_unique<ScriptInspector>(*kind));

    registerInspector(Compositing, std::make_unique<CompositingInspector>());
    registerInspector(MeshSimplify, std::make_unique<MeshSimplifyInspector>());
    registerInspector(Mesh, std::make_unique<MeshInspector>());
    registerInspector(Texture, std::make_unique<TextureInspector>());
    registerInspector(Material, std::make_unique<MaterialInspector>());
    registerInspector(MaterialRef, std::make_unique<MaterialRefInspector>());
    registerInspector(Physics, std::make_unique<PhysicsInspector>());
}

void ComponentUi::draw(Registry& registry, Entity entity) {
    for (const ComponentType* type : drawOrder()) {
        if (!registry.has(entity, *type)) continue;

        Component& component = registry.get(entity, *type);
        ImGui::PushID(&component);
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0, 0, 0, 0.2));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0, 0, 0, 0));
        ImGui::BeginChild("Component", ImVec2{0, 0}, ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY,
                          ImGuiWindowFlags_None);

        ImGui::BeginDisabled(type->isPermanent());
        const bool remove = ui::minusButton("Remove");
        ImGui::EndDisabled();
        ImGui::SameLine();

        if (remove) registry.remove(entity, *type);
        const bool update = !remove && drawComponent(component, registry, entity);

        ImGui::EndChild();
        ImGui::PopStyleColor(3);
        ImGui::PopID();
        if (update) registry.markChanged(*type);
    }
    registry.flush();
}

bool ComponentUi::drawComponent(Component& component, Registry& registry, Entity entity) {
    const ComponentType& type = component.getType();
    ComponentInspector* inspector = findInspector(type);
    if (inspector && inspector->replacesFields()) return inspector->draw(component, registry, entity);

    const std::string header = std::format("{} {}", type.getIcon(), type.getLabel());
    const bool useBullet = !inspector && component.getFields().empty();
    if (!ImGui::CollapsingHeader(header.c_str(), useBullet ? ImGuiTreeNodeFlags_Bullet : ImGuiTreeNodeFlags_None))
        return false;

    bool update = ui::drawGroupedFields(component.getFields(), std::format("##{}", header));
    if (inspector) update |= inspector->draw(component, registry, entity);
    return update;
}

} // namespace ecs
