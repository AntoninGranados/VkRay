#include "component_ui_registry.hpp"

#include <algorithm>

#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "FontAwesome/IconsFontAwesome7.h"

#include "core/core.hpp"
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
        const bool hasPublicFields = std::any_of(fields.begin(), fields.end(), [](const ecs::ComponentField& f){ return !f.isPrivate(); });
        if (!remove && ImGui::CollapsingHeader(header.c_str(), hasPublicFields ? ImGuiTreeNodeFlags_None : ImGuiTreeNodeFlags_Bullet)) {
            for (const ecs::ComponentField& schema : fields) {
                if (schema.isPrivate()) continue;
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

    ui_reg.add(ecs::Sphere);
    ui_reg.add(ecs::Plane);
    ui_reg.add(ecs::Box);
    ui_reg.add(ecs::Collider);
    ui_reg.add(ecs::RigidBody);

    ui_reg.add(MeshRef, [](Component& c, Registry& r, Entity e) {
        auto* meshes = ComponentUiRegistry::get().meshAssets;
        if (!meshes || meshes->empty()) return false;

        bool update = false;
        if (ImGui::CollapsingHeader(ICON_FA_CUBE " Mesh Ref")) {
            ImGui::PushItemWidth(-FLT_MIN);

            int current = c.get<int>("handle");
            const std::string& currentName = (*meshes)[current].getName();
            const char* preview = currentName.empty() ? "Mesh" : currentName.c_str();
            if (ImGui::BeginCombo("##Mesh", preview)) {
                for (int i = 0; i < static_cast<int>(meshes->size()); i++) {
                    const std::string& meshName = (*meshes)[i].getName();
                    const char* display = meshName.empty() ? "Mesh" : meshName.c_str();
                    std::string label = std::string(display) + "##MeshItem" + std::to_string(i);
                    const bool selected = (i == current);
                    if (ImGui::Selectable(label.c_str(), selected)) {
                        c.set<int>("handle", i);
                        update = true;
                    }
                    if (selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();

            ImGui::BeginChild("MeshData", ImVec2{0, 0}, ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_None);
            update |= drawMeshAssetUI((*meshes)[current]);
            ImGui::EndChild();
        }
        return update;
    });

    ui_reg.add(ecs::Camera);

    ui_reg.add(ecs::ThinLens);
    ui_reg.add(ecs::TiltShiftLens);
    ui_reg.add(ecs::GeometricAperture);
    ui_reg.add(ecs::ImageAperture);
    ui_reg.add(ecs::Transform);

    ui_reg.add(MaterialRef, [](Component& c, Registry& r, Entity e) {
        Scene& scene = Core::getScene();
        ecs::Registry& reg = scene.getRegistry();
        const auto& matEnts = reg.storage(ecs::Material).entities();
        if (matEnts.empty()) return false;
        bool update = false;
        if (ImGui::CollapsingHeader(ICON_FA_PALETTE " Material Ref")) {
            ImGui::PushItemWidth(-FLT_MIN);

            int current = c.get<int>("handle");
            if (current < 0 || current >= static_cast<int>(matEnts.size())) current = 0;

            auto matName = [&](int i) -> std::string {
                if (reg.has(matEnts[i], ecs::Name)) {
                    const std::string& n = reg.get(matEnts[i], ecs::Name).get<std::string>("value");
                    if (!n.empty()) return n;
                }
                return "Material";
            };

            std::string previewStr = matName(current);
            if (ImGui::BeginCombo("##Material", previewStr.c_str())) {
                for (int i = 0; i < static_cast<int>(matEnts.size()); i++) {
                    const bool selected = (i == current);
                    std::string label = matName(i) + "##MaterialItem" + std::to_string(i);
                    if (ImGui::Selectable(label.c_str(), selected)) {
                        c.set<int>("handle", i);
                        update = true;
                        current = i;
                    }
                    if (selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();

            const ecs::Entity matEnt = matEnts[current];
            ImGui::BeginChild("##MatEntity", ImVec2{0, 0}, ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY);
            if (ComponentUiRegistry::get().draw(reg, matEnt)) {
                reg.flush();
                Core::requestAccumulationRestart();
                Core::getScene().update();
                update = true;
            }
            ImGui::EndChild();
        }

        return update;
    });

    ui_reg.add(ecs::Material);
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
