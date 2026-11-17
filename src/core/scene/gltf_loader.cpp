#include "gltf_loader.hpp"

#include <cmath>
#include <format>
#include <variant>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "core/ecs/components.hpp"
#include "core/scene/asset/mesh.hpp"
#include "core/scene/scene.hpp"
#include "core/scene/scene_serializer_internal.hpp"
#include "utils/log.hpp"
#include "utils/resources.hpp"
#include "utils/transform_utils.hpp"

bool GltfLoader::load(Scene& scene, const std::filesystem::path& path) {
    fastgltf::Parser parser(fastgltf::Extensions::KHR_materials_ior | fastgltf::Extensions::KHR_materials_transmission |
                            fastgltf::Extensions::KHR_materials_emissive_strength);

    auto data = fastgltf::GltfDataBuffer::FromPath(path);
    if (!data) {
        Log::error("GltfLoader",
                   std::format("Cannot read '{}': {}", path.string(), fastgltf::getErrorMessage(data.error())));
        return false;
    }

    auto parsed = parser.loadGltf(data.get(), path.parent_path(),
                                  fastgltf::Options::LoadExternalBuffers | fastgltf::Options::DecomposeNodeMatrices |
                                      fastgltf::Options::GenerateMeshIndices);
    if (!parsed) {
        Log::error("GltfLoader",
                   std::format("Cannot parse '{}': {}", path.string(), fastgltf::getErrorMessage(parsed.error())));
        return false;
    }
    if (parsed->scenes.empty()) {
        Log::error("GltfLoader", std::format("'{}' has no scene", path.string()));
        return false;
    }

    scene.clear();
    ensureSceneDefaults(scene);

    GltfLoader loader(scene, std::move(parsed.get()), std::filesystem::absolute(path).parent_path());
    loader.loadTextures();
    loader.loadMaterials();
    for (const size_t node : loader.asset.scenes[loader.asset.defaultScene.value_or(0)].nodeIndices)
        loader.loadNode(node, glm::mat4(1.0f));
    if (scene.getRegistry().storage(ecs::Camera).entities().empty()) loader.addDefaultCamera();
    return true;
}

void GltfLoader::setScript(ecs::Entity material, const ecs::ComponentType& type,
                           std::initializer_list<std::pair<const char*, std::vector<float>>> parameters) {
    ecs::Registry& registry = scene.getRegistry();
    registry.add(material, type);
    ShaderPlugin& plugin = registry.get(material, type).payload<ShaderPlugin>("plugin");
    for (const auto& [id, values] : parameters) plugin.setParameter(id, values);
}

std::vector<uint8_t> GltfLoader::readImage(const fastgltf::Image& image) const {
    const auto copy = [](const std::byte* data, size_t size) {
        return std::vector<uint8_t>(reinterpret_cast<const uint8_t*>(data),
                                    reinterpret_cast<const uint8_t*>(data) + size);
    };
    return std::visit(
        fastgltf::visitor{
            [&](const fastgltf::sources::Array& source) { return copy(source.bytes.data(), source.bytes.size()); },
            [&](const fastgltf::sources::BufferView& source) {
                const fastgltf::BufferView& view = asset.bufferViews[source.bufferViewIndex];
                const auto* buffer = std::get_if<fastgltf::sources::Array>(&asset.buffers[view.bufferIndex].data);
                return buffer ? copy(buffer->bytes.data() + view.byteOffset, view.byteLength) : std::vector<uint8_t>{};
            },
            [](const auto&) { return std::vector<uint8_t>{}; },
        },
        image.data);
}

void GltfLoader::loadTextures() {
    ecs::Registry& registry = scene.getRegistry();
    for (size_t i = 0; i < asset.images.size(); i++) {
        const fastgltf::Image& image = asset.images[i];
        const auto* uri = std::get_if<fastgltf::sources::URI>(&image.data);
        const std::filesystem::path file = uri ? directory / uri->uri->fspath() : std::filesystem::path();
        const std::string name = !file.empty()         ? file.filename().string()
                                 : !image.name.empty() ? image.name
                                                       : std::format("Texture {}", i);

        const ecs::Entity entity = scene.createNamedEntity(name, scene.getAssetsRoot());
        registry.add(entity, ecs::Texture);
        ecs::Component& texture = registry.get(entity, ecs::Texture);
        texture.set<bool>("srgb", false);
        texture.set<std::filesystem::path>("path", file);
        texture.payload<TextureAsset>("data").encoded = readImage(image);
        images.push_back(entity);
    }
}

ecs::Entity GltfLoader::textureEntity(const fastgltf::optional<fastgltf::TextureInfo>& info, bool srgb) {
    if (!info) return {};
    if (info->texCoordIndex != 0) Log::warn("GltfLoader", "Only TEXCOORD_0 is supported for textures");

    const auto image = asset.textures[info->textureIndex].imageIndex;
    if (!image) return {};
    if (srgb) scene.getRegistry().get(images[*image], ecs::Texture).set<bool>("srgb", true);
    return images[*image];
}

void GltfLoader::loadMaterials() {
    for (size_t i = 0; i < asset.materials.size(); i++) {
        const fastgltf::Material& material = asset.materials[i];
        const ecs::Entity entity = scene.createNamedEntity(
            material.name.empty() ? std::format("Material {}", i) : material.name, scene.getMaterialsRoot());
        materials.push_back(entity);

        const auto& emissive = material.emissiveFactor;
        if (emissive[0] + emissive[1] + emissive[2] > 0.0f) {
            setScript(entity, ecs::Emissive,
                      {{"albedo", {emissive[0], emissive[1], emissive[2]}},
                       {"emissionStrength", {material.emissiveStrength}}});
            continue;
        }

        const auto& color = material.pbrData.baseColorFactor;
        setScript(entity, ecs::Principled,
                  {{"albedo", {color[0], color[1], color[2]}},
                   {"alpha", {color[3]}},
                   {"roughness", {material.pbrData.roughnessFactor}},
                   {"metalness", {material.pbrData.metallicFactor}},
                   {"ior", {material.ior}},
                   {"transmission", {material.transmission ? material.transmission->transmissionFactor : 0.0f}}});

        ShaderPlugin& plugin = scene.getRegistry().get(entity, ecs::Principled).payload<ShaderPlugin>("plugin");
        const ecs::Entity metallicRoughness = textureEntity(material.pbrData.metallicRoughnessTexture, false);
        plugin.setParameter("albedoTexture", textureEntity(material.pbrData.baseColorTexture, true));
        plugin.setParameter("roughnessTexture", metallicRoughness);
        plugin.setParameter("metalnessTexture", metallicRoughness);
    }
}

ecs::Entity GltfLoader::loadPrimitive(size_t meshIndex, size_t primitiveIndex) {
    const std::pair<size_t, size_t> key{meshIndex, primitiveIndex};
    if (const auto cached = primitives.find(key); cached != primitives.end()) return cached->second;

    const fastgltf::Mesh& mesh = asset.meshes[meshIndex];
    const fastgltf::Primitive& primitive = mesh.primitives[primitiveIndex];
    const auto position = primitive.findAttribute("POSITION");
    if (primitive.type != fastgltf::PrimitiveType::Triangles || position == primitive.attributes.end() ||
        !primitive.indicesAccessor)
        return {};

    std::vector<Vertex> vertices(asset.accessors[position->accessorIndex].count);
    fastgltf::iterateAccessorWithIndex<glm::vec3>(
        asset, asset.accessors[position->accessorIndex],
        [&](glm::vec3 value, size_t index) { vertices[index].position = value; });

    const auto normal = primitive.findAttribute("NORMAL");
    const bool hasNormals = normal != primitive.attributes.end();
    if (hasNormals)
        fastgltf::iterateAccessorWithIndex<glm::vec3>(
            asset, asset.accessors[normal->accessorIndex],
            [&](glm::vec3 value, size_t index) { vertices[index].normal = value; });

    const auto color = primitive.findAttribute("COLOR_0");
    const bool hasColors = color != primitive.attributes.end();
    if (hasColors) {
        const fastgltf::Accessor& accessor = asset.accessors[color->accessorIndex];
        if (accessor.type == fastgltf::AccessorType::Vec4)
            fastgltf::iterateAccessorWithIndex<glm::vec4>(
                asset, accessor, [&](glm::vec4 value, size_t index) { vertices[index].color = glm::vec3(value); });
        else
            fastgltf::iterateAccessorWithIndex<glm::vec3>(
                asset, accessor, [&](glm::vec3 value, size_t index) { vertices[index].color = value; });
    }

    const auto uv = primitive.findAttribute("TEXCOORD_0");
    if (uv != primitive.attributes.end())
        fastgltf::iterateAccessorWithIndex<glm::vec2>(
            asset, asset.accessors[uv->accessorIndex],
            [&](glm::vec2 value, size_t index) { vertices[index].uv = value; });

    std::vector<uint32_t> indices;
    const fastgltf::Accessor& indexAccessor = asset.accessors[*primitive.indicesAccessor];
    indices.reserve(indexAccessor.count);
    fastgltf::iterateAccessor<uint32_t>(asset, indexAccessor, [&](uint32_t index) { indices.push_back(index); });

    const std::string name =
        std::format("{}_{}", mesh.name.empty() ? std::format("Mesh {}", meshIndex) : mesh.name, primitiveIndex);
    const ecs::Entity entity = scene.createNamedEntity(name, scene.getAssetsRoot());
    ecs::Registry& registry = scene.getRegistry();
    registry.add(entity, ecs::Mesh);
    ecs::Component& meshComponent = registry.get(entity, ecs::Mesh);
    meshComponent.set<bool>("smooth", hasNormals);
    meshComponent.payload<MeshAsset>("geometry") = MeshAsset(std::move(vertices), std::move(indices), hasColors);

    primitives[key] = entity;
    return entity;
}

void GltfLoader::addMesh(const fastgltf::Node& node, const std::string& name, const glm::mat4& world) {
    const size_t meshIndex = *node.meshIndex;
    const fastgltf::Mesh& mesh = asset.meshes[meshIndex];
    ecs::Registry& registry = scene.getRegistry();

    for (size_t i = 0; i < mesh.primitives.size(); i++) {
        const ecs::Entity meshAsset = loadPrimitive(meshIndex, i);
        if (meshAsset == ecs::Entity{}) continue;

        const ecs::Entity object = scene.createNamedEntity(
            mesh.primitives.size() > 1 ? std::format("{}_{}", name, i) : name, scene.getObjectsRoot());
        registry.add(object, ecs::MeshRef);
        registry.get(object, ecs::MeshRef).set<ecs::Entity>("handle", meshAsset);

        const auto materialIndex = mesh.primitives[i].materialIndex;
        registry.add(object, ecs::MaterialRef);
        registry.get(object, ecs::MaterialRef)
            .set<ecs::Entity>("handle", materialIndex ? materials[*materialIndex] : scene.getDefaultMaterial());

        applyTransform(registry.get(object, ecs::Transform), world);
    }
}

void GltfLoader::addDefaultCamera() {
    ecs::Registry& registry = scene.getRegistry();
    const ecs::Entity entity = scene.createNamedEntity("Camera", scene.getObjectsRoot());
    registry.add(entity, ecs::Camera);
    ecs::Component& transform = registry.get(entity, ecs::Transform);
    transform.set<glm::vec3>("position", glm::vec3(0.0f, 0.0f, -10.0f));
    transform.set<glm::vec3>("rotation", glm::vec3(0.0f, 180.0f, 0.0f));
}

void GltfLoader::addCamera(const fastgltf::Node& node, const std::string& name, const glm::mat4& world) {
    ecs::Registry& registry = scene.getRegistry();
    const ecs::Entity entity = scene.createNamedEntity(name, scene.getObjectsRoot());
    registry.add(entity, ecs::Camera);
    ecs::Component& camera = registry.get(entity, ecs::Camera);
    camera.set<int>("sensor_fit", std::to_underlying(ecs::CameraSensorFit::Vertical));

    const fastgltf::Camera& source = asset.cameras[*node.cameraIndex];
    if (const auto* perspective = std::get_if<fastgltf::Camera::Perspective>(&source.camera)) {
        camera.set<float>("focal_length",
                          camera.get<float>("sensor_width") * 0.5f / std::tan(perspective->yfov * 0.5f));
    } else if (const auto* orthographic = std::get_if<fastgltf::Camera::Orthographic>(&source.camera)) {
        camera.set<int>("projection", std::to_underlying(ecs::CameraProjection::Orthographic));
        camera.set<float>("sensor_width", orthographic->ymag * 2000.0f);
    }

    applyTransform(registry.get(entity, ecs::Transform), world);
}

void GltfLoader::loadNode(size_t index, const glm::mat4& parent) {
    const fastgltf::Node& node = asset.nodes[index];
    const auto& trs = std::get<fastgltf::TRS>(node.transform);
    const glm::mat4 world =
        parent *
        glm::translate(glm::mat4(1.0f), glm::vec3(trs.translation[0], trs.translation[1], trs.translation[2])) *
        glm::mat4_cast(glm::quat(trs.rotation[3], trs.rotation[0], trs.rotation[1], trs.rotation[2])) *
        glm::scale(glm::mat4(1.0f), glm::vec3(trs.scale[0], trs.scale[1], trs.scale[2]));

    const std::string name = node.name.empty() ? std::format("Node {}", index) : std::string(node.name);
    if (node.meshIndex) addMesh(node, name, world);
    if (node.cameraIndex) addCamera(node, name, world);
    if (node.lightIndex) Log::warn("GltfLoader", std::format("Ignoring light on node '{}'", name));

    for (const size_t child : node.children) loadNode(child, world);
}
