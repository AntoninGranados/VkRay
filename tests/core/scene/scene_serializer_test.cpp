#include "doctest/doctest.h"

#include <filesystem>
#include <fstream>
#include <optional>
#include <string>

#include <glm/gtc/quaternion.hpp>

#include "nlohmann/json.hpp"

#include "core/ecs/components/core.hpp"
#include "core/ecs/components/material.hpp"
#include "core/scene/scene.hpp"
#include "core/scene/scene_serializer.hpp"

namespace {

std::filesystem::path writeSceneFile(const std::string& content) {
    const std::filesystem::path path = std::filesystem::temp_directory_path() / "vkray_scene_serializer_test.json";
    std::ofstream(path) << content;
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

TEST_CASE("SceneSerializer loadCore rejects a version mismatch") {
    const std::filesystem::path path = writeSceneFile(R"({ "version": 2, "Objects": [] })");

    Scene scene;
    scene.initContext();
    CHECK_FALSE(SceneSerializer::loadCore(scene, path.string()));
}

TEST_CASE("SceneSerializer loadCore builds entity naming and hierarchy") {
    const std::filesystem::path path = writeSceneFile(R"({
        "version": 1,
        "Objects": [
            { "name": "Parent", "transform": {}, "children": [
                { "name": "Child", "transform": {} }
            ]}
        ]
    })");

    Scene scene;
    scene.initContext();
    REQUIRE(SceneSerializer::loadCore(scene, path.string()));

    const auto parent = findChildByName(scene, scene.getObjectsRoot(), "Parent");
    REQUIRE(parent.has_value());
    const auto child = findChildByName(scene, *parent, "Child");
    CHECK(child.has_value());
}

TEST_CASE("SceneSerializer loadCore expands a repeat expression") {
    const std::filesystem::path path = writeSceneFile(R"({
        "version": 1,
        "Objects": [
            { "name": "Sphere_{n}", "transform": {}, "repeat": { "count": 3 } }
        ]
    })");

    Scene scene;
    scene.initContext();
    REQUIRE(SceneSerializer::loadCore(scene, path.string()));

    CHECK(findChildByName(scene, scene.getObjectsRoot(), "Sphere_0").has_value());
    CHECK(findChildByName(scene, scene.getObjectsRoot(), "Sphere_1").has_value());
    CHECK(findChildByName(scene, scene.getObjectsRoot(), "Sphere_2").has_value());
}

TEST_CASE("SceneSerializer loadCore expands a grid expression") {
    const std::filesystem::path path = writeSceneFile(R"({
        "version": 1,
        "Objects": [
            { "name": "Cell_{row}_{col}", "transform": {}, "grid": { "rows": 2, "cols": 2 } }
        ]
    })");

    Scene scene;
    scene.initContext();
    REQUIRE(SceneSerializer::loadCore(scene, path.string()));

    CHECK(findChildByName(scene, scene.getObjectsRoot(), "Cell_0_0").has_value());
    CHECK(findChildByName(scene, scene.getObjectsRoot(), "Cell_0_1").has_value());
    CHECK(findChildByName(scene, scene.getObjectsRoot(), "Cell_1_0").has_value());
    CHECK(findChildByName(scene, scene.getObjectsRoot(), "Cell_1_1").has_value());
}

TEST_CASE("SceneSerializer loadCore places a spherical node") {
    const std::filesystem::path path = writeSceneFile(R"({
        "version": 1,
        "Objects": [
            { "name": "OrbitCam", "spherical": { "radius": 10, "azimuth": 0, "elevation": 0 } }
        ]
    })");

    Scene scene;
    scene.initContext();
    REQUIRE(SceneSerializer::loadCore(scene, path.string()));

    const auto entity = findChildByName(scene, scene.getObjectsRoot(), "OrbitCam");
    REQUIRE(entity.has_value());
    REQUIRE(scene.getRegistry().has(*entity, ecs::Transform));
    const glm::vec3 position = scene.getRegistry().get(*entity, ecs::Transform).get<glm::vec3>("position");
    CHECK(position.x == doctest::Approx(0.0f));
    CHECK(position.y == doctest::Approx(0.0f));
    CHECK(position.z == doctest::Approx(10.0f));
}

TEST_CASE("SceneSerializer loadCore aims a spherical node at its target") {
    const std::filesystem::path path = writeSceneFile(R"({
        "version": 1,
        "Objects": [
            { "name": "OrbitCam", "spherical": { "radius": 10, "azimuth": 90, "elevation": 30 } }
        ]
    })");

    Scene scene;
    scene.initContext();
    REQUIRE(SceneSerializer::loadCore(scene, path.string()));

    const auto entity = findChildByName(scene, scene.getObjectsRoot(), "OrbitCam");
    REQUIRE(entity.has_value());
    const ecs::Component& transform = scene.getRegistry().get(*entity, ecs::Transform);
    const glm::vec3 position = transform.get<glm::vec3>("position");
    const glm::quat rotation = glm::quat(glm::radians(transform.get<glm::vec3>("rotation")));
    const glm::vec3 forward = rotation * glm::vec3(0.0f, 0.0f, -1.0f);
    const glm::vec3 toTarget = glm::normalize(-position);
    CHECK(forward.x == doctest::Approx(toTarget.x).epsilon(1e-4));
    CHECK(forward.y == doctest::Approx(toTarget.y).epsilon(1e-4));
    CHECK(forward.z == doctest::Approx(toTarget.z).epsilon(1e-4));
}

TEST_CASE("SceneSerializer loadCore applies keyframes at frame zero") {
    const std::filesystem::path path = writeSceneFile(R"({
        "version": 1,
        "Objects": [
            { "name": "Anim", "transform": { "position": { "anim": [
                { "frame": 0, "value": [1, 2, 3] },
                { "frame": 10, "value": [5, 0, 0] }
            ]}}}
        ]
    })");

    Scene scene;
    scene.initContext();
    REQUIRE(SceneSerializer::loadCore(scene, path.string()));

    const auto entity = findChildByName(scene, scene.getObjectsRoot(), "Anim");
    REQUIRE(entity.has_value());
    const glm::vec3 position = scene.getRegistry().get(*entity, ecs::Transform).get<glm::vec3>("position");
    CHECK(position.x == doctest::Approx(1.0f));
    CHECK(position.y == doctest::Approx(2.0f));
    CHECK(position.z == doctest::Approx(3.0f));
}

TEST_CASE("SceneSerializer loadCore resolves a cross-entity name reference") {
    const std::filesystem::path path = writeSceneFile(R"({
        "version": 1,
        "Materials": [
            { "name": "Material_A", "diffuse": { "albedo": [1, 0, 0] } }
        ],
        "Objects": [
            { "name": "Obj", "transform": {}, "material_ref": { "handle": "Material_A" } }
        ]
    })");

    Scene scene;
    scene.initContext();
    REQUIRE(SceneSerializer::loadCore(scene, path.string()));

    const auto material = findChildByName(scene, scene.getMaterialsRoot(), "Material_A");
    const auto obj = findChildByName(scene, scene.getObjectsRoot(), "Obj");
    REQUIRE(material.has_value());
    REQUIRE(obj.has_value());
    REQUIRE(scene.getRegistry().has(*obj, ecs::MaterialRef));

    const ecs::Entity handle = scene.getRegistry().get(*obj, ecs::MaterialRef).get<ecs::Entity>("handle");
    CHECK(handle == *material);
}

TEST_CASE("SceneSerializer save round-trips the golden scene") {
    const std::filesystem::path originalPath = "tests/golden/scene.json";
    const std::filesystem::path savedPathA =
        std::filesystem::temp_directory_path() / "vkray_scene_serializer_roundtrip_a.json";
    const std::filesystem::path savedPathB =
        std::filesystem::temp_directory_path() / "vkray_scene_serializer_roundtrip_b.json";

    Scene sceneA;
    sceneA.initContext();
    REQUIRE(SceneSerializer::loadCore(sceneA, originalPath.string()));
    REQUIRE(SceneSerializer::save(sceneA, savedPathA.string()));

    Scene sceneB;
    sceneB.initContext();
    REQUIRE(SceneSerializer::loadCore(sceneB, savedPathA.string()));
    REQUIRE(SceneSerializer::save(sceneB, savedPathB.string()));

    std::ifstream savedFileA(savedPathA);
    std::ifstream savedFileB(savedPathB);
    REQUIRE(savedFileA.is_open());
    REQUIRE(savedFileB.is_open());

    const nlohmann::ordered_json a = nlohmann::ordered_json::parse(savedFileA);
    const nlohmann::ordered_json b = nlohmann::ordered_json::parse(savedFileB);
    CHECK(a == b);
}
