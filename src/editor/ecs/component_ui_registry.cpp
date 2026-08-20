#include "component_ui_registry.hpp"

#include "FontAwesome/IconsFontAwesome7.h"

#include "core/core.hpp"
#include "core/ecs/systems/mesh_system.hpp"
#include "core/scene/asset/mesh.hpp"
#include "core/scene/scene.hpp"
#include "editor/field_ui.hpp"
#include "editor/ui_utils.hpp"

namespace ecs {

ComponentUiRegistry& ComponentUiRegistry::get() {
    static ComponentUiRegistry r;
    return r;
}

bool ComponentUiRegistry::drawField(Component& component, const ecs::ComponentField& schema) {
    return ui::drawField(component.getField(schema.getId()), "##" + schema.getId());
}

void ComponentUiRegistry::add(const ecs::ComponentType& componentType) {
    drawers.emplace_back([type = componentType](Registry& registry, Entity e) {
        if (!registry.has(e, type)) return false;

        Component& component = registry.get(e, type);
        const std::string header = component.getType().getIcon() + " " + component.getType().getLabel();
        const auto& fields = component.getType().getFields();

        bool remove = ComponentUiRegistry::beginDraw(&component);
        if (remove) registry.remove(e, type);
        bool update = false;
        const bool hasFields = !fields.empty();
        if (!remove && ImGui::CollapsingHeader(header.c_str(), hasFields ? ImGuiTreeNodeFlags_None : ImGuiTreeNodeFlags_Bullet)) {
            for (const ecs::ComponentField& schema : fields) {
                if (schema.isAnimatable()) ui::drawKeyframeButton(e, component, schema.getId());
                update |= ComponentUiRegistry::drawField(component, schema);
            }
        }
        ComponentUiRegistry::endDraw();
        return remove || update;
    });
}

void ComponentUiRegistry::init() {
    static bool init = false;
    if (init) return;
    init = true;

    auto& ui_reg = ComponentUiRegistry::get();

    ui_reg.add(ecs::Name);
    ui_reg.add(ecs::Material);

    ui_reg.add(ecs::Sphere);
    ui_reg.add(ecs::Plane);
    ui_reg.add(ecs::Box);
    ui_reg.add(ecs::Collider);
    ui_reg.add(ecs::RigidBody);

    ui_reg.add(ecs::Mesh, [](Component& c, Registry& r, Entity e) {
        Scene& scene = Core::getScene();
        bool update = false;
        if (ImGui::CollapsingHeader(ICON_FA_CUBE " Mesh")) {
            for (const ecs::ComponentField& schema : c.getType().getFields()) {
                update |= ComponentUiRegistry::drawField(c, schema);
            }
            if (const MeshAsset* mesh = scene.getMeshAsset(e)) {
                ImGui::BeginChild("MeshData", ImVec2{0, 0}, ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_None);
                ImGui::Text("Vertices: %zu", mesh->getVertices().size());
                ImGui::Text("Faces:    %zu", mesh->getIndices().size() / 3);
                ImGui::EndChild();
            }
        }
        return update;
    });

    ui_reg.add(ecs::MeshSimplify, [](Component& c, Registry& r, Entity e) {
        bool update = false;
        if (ImGui::CollapsingHeader(ICON_FA_CUBE " Mesh Simplify")) {
            float ratio = c.get<float>("ratio");
            ImGui::PushItemWidth(-FLT_MIN);
            if (ImGui::SliderFloat("##Ratio", &ratio, 0.05f, 1.0f, "%.2f")) {
                c.set<float>("ratio", ratio);
                ecs::requestMeshSimplify(r, e, ratio);
                update = true;
            }
            ImGui::PopItemWidth();
            if (ImGui::BeginTable("##SimplifyButtons", 2, ImGuiTableFlags_SizingStretchSame)) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                if (ImGui::Button("Apply", ImVec2(-FLT_MIN, 0.0f))) {
                    ecs::applyMeshSimplification(r, e);
                    update = true;
                }
                ImGui::TableSetColumnIndex(1);
                ui::PushCancelStyleColor();
                if (ImGui::Button("Revert", ImVec2(-FLT_MIN, 0.0f))) {
                    ecs::revertMeshSimplification(r, e);
                    update = true;
                }
                ui::PopCancelStyleColor();
                ImGui::EndTable();
            }
        }
        return update;
    });

    ui_reg.add(ecs::MeshRef);

    ui_reg.add(ecs::Camera);

    ui_reg.add(ecs::ThinLens);
    ui_reg.add(ecs::TiltShiftLens);
    ui_reg.add(ecs::GeometricAperture);
    ui_reg.add(ecs::ImageAperture);
    ui_reg.add(ecs::Transform);

    ui_reg.add(ecs::MaterialRef);

    ui_reg.add(ecs::Diffuse);
    ui_reg.add(ecs::Emissive);
    ui_reg.add(ecs::Metal);
    ui_reg.add(ecs::Glossy);
    ui_reg.add(ecs::Dielectric);
    ui_reg.add(ecs::Volume);
    ui_reg.add(ecs::Principled);
    ui_reg.add(ecs::ProgrammableMaterial);
}

} // namespace ecs
