#include "mesh_system.hpp"

#include <algorithm>

#include "core/core.hpp"
#include "core/ecs/components.hpp"
#include "core/scene/asset/mesh.hpp"
#include "core/scene/asset/mesh_simplify.hpp"
#include "core/scene/scene.hpp"

namespace ecs {

void requestMeshSimplify(Entity meshEntity, float ratio) {
    Registry& registry = Core::getScene().getRegistry();
    if (!registry.has(meshEntity, Mesh) || !registry.has(meshEntity, MeshSimplify)) return;

    ratio = std::clamp(ratio, 0.05f, 1.0f);

    MeshAsset& live = registry.get(meshEntity, Mesh).payload<MeshAsset>("geometry");
    MeshAsset& original = registry.get(meshEntity, MeshSimplify).payload<MeshAsset>("original");
    if (original.getVertices().empty())
        original = live;

    live = (ratio >= 1.0f - 1e-6f) ? original : simplifyMesh(original, ratio);
    Core::markDirty();
}

void applyMeshSimplification(Entity meshEntity) {
    Registry& registry = Core::getScene().getRegistry();
    if (!registry.has(meshEntity, Mesh) || !registry.has(meshEntity, MeshSimplify)) return;

    const MeshAsset& live = registry.get(meshEntity, Mesh).payload<MeshAsset>("geometry");
    registry.get(meshEntity, MeshSimplify).payload<MeshAsset>("original") = live;
    registry.get(meshEntity, MeshSimplify).set<float>("ratio", 1.0f);
    Core::markDirty();
}

void revertMeshSimplification(Entity meshEntity) {
    Registry& registry = Core::getScene().getRegistry();
    if (!registry.has(meshEntity, Mesh) || !registry.has(meshEntity, MeshSimplify)) return;

    const MeshAsset& original = registry.get(meshEntity, MeshSimplify).payload<MeshAsset>("original");
    if (original.getVertices().empty()) return;

    registry.get(meshEntity, Mesh).payload<MeshAsset>("geometry") = original;
    registry.get(meshEntity, MeshSimplify).set<float>("ratio", 1.0f);
    Core::markDirty();
}

} // namespace ecs
