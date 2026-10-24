#include "scene.hpp"

#include <filesystem>
#include <format>
#include <optional>
#include <utility>

#include "core/ecs/components/core.hpp"
#include "core/ecs/entity.hpp"
#include "utils/log.hpp"

void Scene::clear() {
    resetSceneState();
    addDefaultAssets();
}

const std::vector<ecs::Entity>& Scene::getChildren(ecs::Entity parent) const { return registry.getChildren(parent); }

ecs::Entity Scene::getEnvironment() const { return findSceneRootChild(ecs::Environment); }

ecs::Entity Scene::getCompositing() const { return findSceneRootChild(ecs::Compositing); }

ecs::Entity Scene::findSceneRootChild(const ecs::ComponentType& type) const {
    for (const ecs::Entity& e : registry.getChildren(roots.sceneRoot))
        if (registry.has(e, type)) return e;
    return {};
}

ecs::Entity Scene::loadMeshAsset(std::string name, const std::string& path, bool smooth) {
    std::optional<MeshAsset> asset = MeshAsset::load(path);
    if (!asset) return {};

    ecs::Entity e = createNamedEntity(std::move(name), roots.assetsRoot);
    registry.add(e, ecs::Mesh);
    ecs::Component& meshComponent = registry.get(e, ecs::Mesh);
    meshComponent.set<std::filesystem::path>("path", path);
    meshComponent.set<bool>("smooth", smooth);
    meshComponent.payload<MeshAsset>("geometry") = std::move(*asset);

    Log::success("Scene", std::format("Loaded mesh: {}", path));
    return e;
}

ecs::Entity Scene::createNamedEntity(std::string name, ecs::Entity parent) {
    ecs::Entity e = registry.createEntity(parent);
    registry.add(e, ecs::Name);
    registry.get(e, ecs::Name).set<std::string>("value", name);
    return e;
}

void Scene::resetSceneState() {
    registry.clear();
    animationStore.clear();
}

void Scene::addDefaultAssets() {
    roots.sceneRoot = createNamedEntity("Scene");
    roots.materialsRoot = createNamedEntity("Materials");
    roots.assetsRoot = createNamedEntity("Assets");
    roots.objectsRoot = createNamedEntity("Objects");
    roots.internalsRoot = createNamedEntity("Internals");

    defaultMaterial = createNamedEntity("Default Material", roots.materialsRoot);
    registry.add(defaultMaterial, ecs::Diffuse);
    registry.get(defaultMaterial, ecs::Diffuse).set<glm::vec3>("albedo", glm::vec3(0.5f, 0.0f, 0.5f));

    defaultMesh = createNamedEntity("Default Cube", roots.assetsRoot);
    registry.add(defaultMesh, ecs::Mesh);
    registry.get(defaultMesh, ecs::Mesh).payload<MeshAsset>("geometry") = makeDefaultMeshAsset();

    const ecs::Entity environment = createNamedEntity("Environment", roots.sceneRoot);
    registry.add(environment, ecs::Environment);
    registry.add(environment, ecs::Physics);

    const ecs::Entity compositing = createNamedEntity("Compositing", roots.sceneRoot);
    registry.add(compositing, ecs::Compositing);

    for (const ecs::Entity entity : {roots.sceneRoot, roots.materialsRoot, roots.assetsRoot, roots.objectsRoot,
                                     roots.internalsRoot, defaultMaterial, defaultMesh, environment, compositing})
        registry.add(entity, ecs::Locked);

    activeCamera.reset();
}

MeshAsset* Scene::getMeshAsset(ecs::Entity e) {
    return registry.has(e, ecs::Mesh) ? &registry.get(e, ecs::Mesh).payload<MeshAsset>("geometry") : nullptr;
}

const MeshAsset* Scene::getMeshAsset(ecs::Entity e) const {
    return registry.has(e, ecs::Mesh) ? &registry.get(e, ecs::Mesh).payload<MeshAsset>("geometry") : nullptr;
}
