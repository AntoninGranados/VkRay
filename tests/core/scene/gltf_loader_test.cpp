#include "doctest/doctest.h"

#include <filesystem>
#include <fstream>
#include <optional>
#include <string>

#include "core/ecs/components/camera.hpp"
#include "core/ecs/components/core.hpp"
#include "core/ecs/components/geometry.hpp"
#include "core/ecs/components/material.hpp"
#include "core/scene/scene.hpp"
#include "core/scene/scene_serializer.hpp"

namespace {

std::filesystem::path writeGltf() {
    const std::filesystem::path path = std::filesystem::temp_directory_path() / "vkray_gltf_loader_test.gltf";
    std::ofstream(path) << R"({
        "asset": { "version": "2.0" },
        "scene": 0,
        "scenes": [{ "nodes": [0, 1] }],
        "nodes": [
            { "name": "Triangle", "mesh": 0, "translation": [1, 2, 3] },
            { "name": "View", "camera": 0 }
        ],
        "meshes": [{ "primitives": [{ "attributes": { "POSITION": 0 }, "indices": 1, "material": 0 }] }],
        "materials": [{ "name": "Red", "pbrMetallicRoughness": { "baseColorFactor": [1, 0, 0, 1], "roughnessFactor": 0.25 } }],
        "cameras": [{ "type": "perspective", "perspective": { "yfov": 0.8, "znear": 0.1 } }],
        "buffers": [{ "byteLength": 44, "uri": "data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAgD8AAAAAAAABAAIAAAA=" }],
        "bufferViews": [
            { "buffer": 0, "byteOffset": 0, "byteLength": 36 },
            { "buffer": 0, "byteOffset": 36, "byteLength": 6 }
        ],
        "accessors": [
            { "bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3", "min": [0, 0, 0], "max": [1, 1, 0] },
            { "bufferView": 1, "componentType": 5123, "count": 3, "type": "SCALAR" }
        ]
    })";
    return path;
}

std::optional<ecs::Entity> findChildByName(const Scene& scene, ecs::Entity root, const std::string& name) {
    for (const ecs::Entity& child : scene.getChildren(root)) {
        if (scene.getRegistry().has(child, ecs::Name) &&
            scene.getRegistry().get(child, ecs::Name).get<std::string>("value") == name)
            return child;
    }
    return std::nullopt;
}

} // namespace

TEST_CASE("SceneSerializer loadCore reads a glTF file") {
    Scene scene;
    REQUIRE(SceneSerializer::loadCore(scene, writeGltf().string()));
    const ecs::Registry& registry = scene.getRegistry();

    const auto triangle = findChildByName(scene, scene.getObjectsRoot(), "Triangle");
    REQUIRE(triangle.has_value());
    REQUIRE(registry.has(*triangle, ecs::MeshRef));
    const glm::vec3 position = registry.get(*triangle, ecs::Transform).get<glm::vec3>("position");
    CHECK(position.x == doctest::Approx(1.0f));
    CHECK(position.y == doctest::Approx(2.0f));
    CHECK(position.z == doctest::Approx(3.0f));

    const ecs::Entity meshAsset = registry.get(*triangle, ecs::MeshRef).get<ecs::Entity>("handle");
    REQUIRE(scene.getMeshAsset(meshAsset) != nullptr);
    CHECK(scene.getMeshAsset(meshAsset)->getIndices().size() == 3);

    const auto red = findChildByName(scene, scene.getMaterialsRoot(), "Red");
    REQUIRE(red.has_value());
    CHECK(registry.get(*triangle, ecs::MaterialRef).get<ecs::Entity>("handle") == *red);
    CHECK(registry.has(*red, ecs::Principled));

    const auto view = findChildByName(scene, scene.getObjectsRoot(), "View");
    REQUIRE(view.has_value());
    CHECK(registry.has(*view, ecs::Camera));
}
