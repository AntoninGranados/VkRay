#pragma once

#include <filesystem>
#include <initializer_list>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <fastgltf/core.hpp>
#include <glm/glm.hpp>

#include "core/ecs/components/component_type.hpp"
#include "core/ecs/entity.hpp"

class Scene;

class GltfLoader {
public:
    static bool handles(const std::filesystem::path& path) {
        return path.extension() == ".gltf" || path.extension() == ".glb";
    }
    static bool load(Scene& scene, const std::filesystem::path& path);

private:
    GltfLoader(Scene& scene, fastgltf::Asset&& asset, std::filesystem::path directory)
        : scene(scene), asset(std::move(asset)), directory(std::move(directory)) {}

    void loadTextures();
    std::vector<uint8_t> readImage(const fastgltf::Image& image) const;
    template <typename Optional> ecs::Entity textureEntity(const Optional& info, bool srgb);
    void loadMaterials();
    void loadNode(size_t index, const glm::mat4& parent);
    void addMesh(const fastgltf::Node& node, const std::string& name, const glm::mat4& world);
    void addCamera(const fastgltf::Node& camera, const std::string& name, const glm::mat4& world);
    ecs::Entity loadPrimitive(size_t meshIndex, size_t primitiveIndex);
    void setScript(ecs::Entity material, const ecs::ComponentType& type,
                   std::initializer_list<std::pair<const char*, std::vector<float>>> parameters);

    Scene& scene;
    fastgltf::Asset asset;
    std::filesystem::path directory;
    std::vector<ecs::Entity> images;
    std::vector<ecs::Entity> materials;
    std::map<std::pair<size_t, size_t>, ecs::Entity> primitives;
};
