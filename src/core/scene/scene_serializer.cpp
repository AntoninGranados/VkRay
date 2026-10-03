#include "scene_serializer.hpp"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include "nlohmann/json.hpp"

#include "core/ecs/components/compositing.hpp"
#include "core/ecs/components/core.hpp"
#include "core/ecs/components/environment.hpp"
#include "core/ecs/components/physics.hpp"
#include "core/fields/field_serializer.hpp"
#include "scene.hpp"
#include "scene_serializer_internal.hpp"
#include "utils/json_dsl.hpp"
#include "utils/log.hpp"
#include "utils/resources.hpp"

using json = nlohmann::ordered_json;

namespace {

constexpr std::pair<const char*, Interpolation> kInterpolations[] = {
    {"linear", Interpolation::Linear},  {"step", Interpolation::Step},        {"cubic", Interpolation::Cubic},
    {"ease_in", Interpolation::EaseIn}, {"ease_out", Interpolation::EaseOut}, {"ease_in_out", Interpolation::EaseInOut},
};

template <typename T, size_t N>
T fromStr(const std::pair<const char*, T> (&table)[N], const std::string& s, T fallback) {
    for (const auto& [name, val] : table)
        if (s == name) return val;
    return fallback;
}
template <typename T, size_t N> const char* toStr(const std::pair<const char*, T> (&table)[N], T e) {
    for (const auto& [name, val] : table)
        if (e == val) return name;
    std::unreachable();
}

json serializeKeyframes(const std::map<int, Keyframe>& kfs) {
    json arr = json::array();
    for (const auto& [frame, kf] : kfs) {
        json kfj;
        kfj["frame"] = frame;
        kfj["value"] = fieldValueToJson(kf.getValue());
        if (kf.getInterpolation() != Interpolation::Linear) kfj["ease"] = toStr(kInterpolations, kf.getInterpolation());
        arr.push_back(kfj);
    }
    return arr;
}

json serializeField(const Field& f, const std::map<int, Keyframe>& kfs) {
    if (!kfs.empty()) return json{{"anim", serializeKeyframes(kfs)}};
    return fieldValueToJson(f);
}

void applyKeyframes(const json& anim, FieldType type, const std::string& fieldId,
                    const std::function<void(int, FieldValue, Interpolation)>& insert) {
    for (const auto& kf : anim) {
        if (!kf.contains("frame") || !kf.contains("value")) {
            Log::error("SceneSerializer", std::format("Keyframe for '{}' missing 'frame' or 'value'", fieldId));
            continue;
        }
        const Interpolation interp =
            kf.contains("ease") ? fromStr(kInterpolations, kf["ease"].get<std::string>(), Interpolation::Linear)
                                : Interpolation::Linear;
        insert(kf["frame"].get<int>(), fieldValueFromJson(kf["value"], type), interp);
    }
}

std::filesystem::path makeSceneAbsolute(const std::filesystem::path& path,
                                        const std::filesystem::path& sceneDirectory) {
    if (path.empty() || path.is_absolute() || Resources::isBuiltin(path)) return path;
    return (sceneDirectory / path).lexically_normal();
}

std::filesystem::path makeSceneRelative(const std::filesystem::path& path,
                                        const std::filesystem::path& sceneDirectory) {
    if (path.empty() || Resources::isBuiltin(path)) return path;
    return std::filesystem::absolute(path).lexically_normal().lexically_proximate(sceneDirectory);
}

json serializeComponent(ecs::Component& comp, const AnimationStore& animStore, const ecs::Registry& registry,
                        const std::filesystem::path& sceneDirectory) {
    json j = json::object();
    comp.forEachField([&](Field& f) {
        const std::string id = f.getId().generic_string();
        if (f.getType() == FieldType::Entity) {
            const ecs::Entity referenced = f.get<ecs::Entity>();
            if (referenced != ecs::Entity{} && registry.has(referenced, ecs::Name)) {
                const std::string name = registry.get(referenced, ecs::Name).get<std::string>("value");
                if (!name.empty()) j[id] = name;
            }
            return;
        }
        if (f.getType() == FieldType::Path) {
            j[id] = makeSceneRelative(f.get<std::filesystem::path>(), sceneDirectory).generic_string();
            return;
        }
        j[id] = serializeField(f, animStore.keyframes(f));
    });
    return j;
}

json serializeCompositingPasses(ecs::Component& comp, const std::filesystem::path& sceneDirectory) {
    json arr = json::array();
    for (ecs::CompositingPassEntry& pass : comp.payload<ecs::CompositingPasses>("passes").passes) {
        json entry;
        entry["name"] = pass.name;
        entry["path"] = makeSceneRelative(pass.path, sceneDirectory).generic_string();
        json params = json::object();
        for (const Field& f : pass.plugin->getComponent().getFields())
            params[f.getId().generic_string()] = fieldValueToJson(f);
        entry["params"] = params;
        arr.push_back(entry);
    }
    return json{{"passes", arr}};
}

void loadCompositingPasses(const json& value, ecs::Component& comp, ecs::Entity entity, const ResolveCtx& ctx,
                           std::vector<DeferredCompositingParams>& deferred) {
    ecs::CompositingPasses& list = comp.payload<ecs::CompositingPasses>("passes");
    if (!value.contains("passes") || !value["passes"].is_array()) return;

    for (const json& entry : value["passes"]) {
        ecs::CompositingPassEntry pass;
        pass.name = entry.value("name", std::string("Pass"));
        pass.path = entry.value("path", std::string());
        deferred.push_back({entity, list.passes.size(), entry.value("params", json::object()), ctx});
        list.passes.push_back(std::move(pass));
    }
}

void spawnSpherical(const json& node, ecs::Entity e, ecs::Registry& registry, const ResolveCtx& ctx) {
    if (!node.contains("spherical")) return;
    const auto& sj = node["spherical"];
    const float radius = resolveFloat(sj["radius"], ctx);
    const float azDeg = resolveFloat(sj["azimuth"], ctx);
    const float elDeg = resolveFloat(sj["elevation"], ctx);
    const glm::vec3 target = sj.contains("target") ? resolveVec3(sj["target"], ctx) : glm::vec3{0, 0, 0};

    const float az = glm::radians(azDeg);
    const float el = glm::radians(elDeg);
    const glm::vec3 pos =
        target + radius * glm::vec3(std::cos(el) * std::sin(az), std::sin(el), std::cos(el) * std::cos(az));
    const glm::vec3 dir = glm::normalize(target - pos);

    auto ttype = ecs::ComponentType::find("transform");
    if (!ttype || !registry.add(e, ttype->get())) return;
    auto& t = registry.get(e, ttype->get());
    t.set<glm::vec3>("position", pos);
    t.set<glm::vec3>("rotation", {glm::degrees(std::asin(glm::clamp(dir.y, -1.0f, 1.0f))),
                                  glm::degrees(std::atan2(-dir.x, -dir.z)), 0.0f});
}

bool hasPluginPayload(const ecs::ComponentType& type) {
    for (const ecs::ComponentPayload& p : type.getPayloads())
        if (p.asComponent) return true;
    return false;
}

void spawnComponents(const json& node, ecs::Entity e, const ResolveCtx& resolveCtx, SpawnContext& spawn) {
    for (const auto& [key, value] : node.items()) {
        if (key == "name" || key == "children" || key == "repeat" || key == "grid" || key == "spherical") continue;
        auto type = ecs::ComponentType::find(key);
        if (!type) {
            Log::warn("SceneSerializer", std::format("Unknown component '{}'", key));
            continue;
        }
        if (!spawn.registry.add(e, type->get())) {
            Log::warn("SceneSerializer", std::format("Component '{}' cannot be added to '{}' and was skipped", key,
                                                     node.value("name", std::string("unnamed"))));
            continue;
        }
        if (value.is_object()) {
            if (type->get() == ecs::Compositing) {
                loadCompositingPasses(value, spawn.registry.get(e, type->get()), e, resolveCtx,
                                      spawn.deferredCompositingParams);
                continue;
            }
            for (const auto& field : spawn.registry.get(e, type->get()).getFields()) {
                if (field.getType() != FieldType::Entity) continue;
                const std::string id = field.getId().generic_string();
                if (!value.contains(id) || !value[id].is_string()) continue;
                spawn.deferredEntityFields.push_back(
                    {e, &type->get(), id, resolveTemplate(value[id].get<std::string>(), resolveCtx)});
            }
            applyComponent(value, spawn.registry.get(e, type->get()), spawn.animStore, resolveCtx);
            if (hasPluginPayload(type->get()))
                spawn.deferredPluginParams.push_back({e, &type->get(), value, resolveCtx});
            else
                warnUnknownFields(value, spawn.registry.get(e, type->get()));
        }
    }
}

void loadNode(const json& node, ecs::Entity parent, const ResolveCtx& resolveCtx, SpawnContext& spawn) {
    auto spawnOne = [&](const ResolveCtx& oneCtx) {
        ecs::Entity e = spawn.registry.createEntity(parent);

        if (node.contains("name") && node["name"].is_string()) {
            const std::string name = resolveTemplate(node["name"].get<std::string>(), oneCtx);
            spawn.registry.add(e, ecs::Name);
            spawn.registry.get(e, ecs::Name).set<std::string>("value", name);
        }

        spawnComponents(node, e, oneCtx, spawn);
        spawnSpherical(node, e, spawn.registry, oneCtx);

        if (node.contains("children") && node["children"].is_array()) {
            for (const auto& child : node["children"]) {
                if (child.is_object()) loadNode(child, e, oneCtx, spawn);
            }
        }
    };

    if (node.contains("repeat")) {
        const int count = node["repeat"].value("count", 1);
        for (int n = 0; n < count; n++) spawnOne(ResolveCtx{resolveCtx.rng, {{"n", {n, count}}}});
    } else if (node.contains("grid")) {
        const auto& g = node["grid"];
        const int rows = g.value("rows", 1);
        const int cols = g.value("cols", 1);
        for (int r = 0; r < rows; r++)
            for (int c = 0; c < cols; c++)
                spawnOne(ResolveCtx{resolveCtx.rng,
                                    {{"row", {r, rows}}, {"col", {c, cols}}, {"n", {r * cols + c, rows * cols}}}});
    } else {
        spawnOne(resolveCtx);
    }
}

} // namespace

void applyComponent(const json& obj, ecs::Component& comp, AnimationStore& animStore, const ResolveCtx& ctx) {
    comp.forEachField([&](Field& f) {
        const std::string id = f.getId().generic_string();
        if (!obj.contains(id)) return;
        if (f.getType() == FieldType::Entity) return;
        const json& val = obj[id];
        if (val.is_object() && val.contains("anim") && val["anim"].is_array())
            applyKeyframes(val["anim"], f.getType(), id, [&](int frame, FieldValue value, Interpolation interp) {
                animStore.insert(f, value, frame, interp);
            });
        else
            applyField(val, f, ctx);
    });
}

void warnUnknownFields(const json& obj, ecs::Component& comp) {
    std::unordered_set<std::string> knownIds;
    comp.forEachField([&](Field& f) { knownIds.insert(f.getId().generic_string()); });
    for (const auto& [key, value] : obj.items())
        if (!knownIds.contains(key))
            Log::warn("SceneSerializer",
                      std::format("Unknown field '{}' on component '{}'", key, comp.getType().getId()));
}

std::optional<json> parseSceneFile(const std::string& path) {
    const std::optional<std::string> source = Resources::read(path);
    if (!source) {
        Log::error("SceneSerializer", std::format("Cannot open scene: {}", path));
        return std::nullopt;
    }
    try {
        return json::parse(*source, nullptr, true, true);
    } catch (const json::parse_error& e) {
        Log::error("SceneSerializer", std::format("Scene parse error: {}", e.what()));
        return std::nullopt;
    }
}

uint32_t resolveSeed(const json& j, std::optional<uint32_t> forceSeed) {
    return forceSeed.value_or(j.contains("seed") ? j["seed"].get<uint32_t>()
                                                 : static_cast<uint32_t>(std::random_device{}()));
}

void loadSection(const json& j, const std::string& key, ecs::Entity root, const ResolveCtx& resolveCtx,
                 SpawnContext& spawn) {
    if (!j.contains(key) || !j[key].is_array()) return;
    for (const auto& child : j[key])
        if (child.is_object()) loadNode(child, root, resolveCtx, spawn);
}

std::unordered_map<std::string, ecs::Entity> buildEntityNameMap(const Scene& scene, const ecs::Registry& registry) {
    std::unordered_map<std::string, ecs::Entity> entityByName;
    for (const ecs::Entity& entity : scene.getChildren(scene.getMaterialsRoot())) {
        const std::string name = registry.get(entity, ecs::Name).get<std::string>("value");
        if (!name.empty()) entityByName[name] = entity;
    }
    for (const ecs::Entity& entity : scene.getChildren(scene.getAssetsRoot())) {
        const std::string name = registry.get(entity, ecs::Name).get<std::string>("value");
        if (!name.empty()) entityByName[name] = entity;
    }
    return entityByName;
}

void resolveDeferredEntityFields(ecs::Registry& registry, const std::vector<DeferredEntityField>& deferredEntityFields,
                                 const std::unordered_map<std::string, ecs::Entity>& entityByName) {
    for (const DeferredEntityField& deferred : deferredEntityFields) {
        const auto found = entityByName.find(deferred.entityName);
        if (found != entityByName.end())
            registry.get(deferred.entity, *deferred.componentType).set<ecs::Entity>(deferred.fieldId, found->second);
        else
            Log::warn("SceneSerializer",
                      std::format("Entity '{}' not found for field '{}'", deferred.entityName, deferred.fieldId));
    }
}

void resolveScenePaths(ecs::Registry& registry, const std::filesystem::path& sceneDirectory) {
    for (const ecs::ComponentType& type : ecs::ComponentType::all()) {
        for (const ecs::Entity entity : registry.storage(type).entities()) {
            ecs::Component& comp = registry.get(entity, type);
            comp.forEachField([&](Field& f) {
                if (f.getType() == FieldType::Path)
                    f.set(makeSceneAbsolute(f.get<std::filesystem::path>(), sceneDirectory));
            });
            if (type == ecs::Compositing)
                for (ecs::CompositingPassEntry& pass : comp.payload<ecs::CompositingPasses>("passes").passes)
                    pass.path = makeSceneAbsolute(pass.path, sceneDirectory);
        }
    }
}

void reloadMeshAssets(ecs::Registry& registry, const Scene& scene) {
    for (const ecs::Entity entity : scene.getChildren(scene.getAssetsRoot())) {
        if (!registry.has(entity, ecs::Mesh)) continue;
        ecs::Component& meshComp = registry.get(entity, ecs::Mesh);
        const std::filesystem::path meshPath = meshComp.get<std::filesystem::path>("path");
        if (meshPath.empty()) continue;
        std::optional<MeshAsset> asset = MeshAsset::load(meshPath.string());
        if (asset) {
            meshComp.payload<MeshAsset>("geometry") = std::move(*asset);
        } else {
            Log::error("SceneSerializer", std::format("Failed to load mesh: {}", meshPath.string()));
        }
    }
}

void replaceSceneRootIfProvided(const json& j, Scene& scene) {
    if (!j.contains("Scene") || !j["Scene"].is_array() || j["Scene"].empty()) return;
    ecs::Registry& registry = scene.getRegistry();
    const std::vector<ecs::Entity> children = scene.getChildren(scene.getSceneRoot());
    for (const ecs::Entity& child : children) registry.destroyEntity(child);
}

void ensureSceneDefaults(Scene& scene) {
    ecs::Registry& registry = scene.getRegistry();
    if (scene.getEnvironment() == ecs::Entity{}) {
        const ecs::Entity e = scene.createNamedEntity("Environment", scene.getSceneRoot());
        registry.add(e, ecs::Environment);
    }
    if (scene.getCompositing() == ecs::Entity{}) {
        const ecs::Entity e = scene.createNamedEntity("Compositing", scene.getSceneRoot());
        registry.add(e, ecs::Compositing);
    }
    registry.add(scene.getEnvironment(), ecs::Physics);
    registry.add(scene.getEnvironment(), ecs::Locked);
    registry.add(scene.getCompositing(), ecs::Locked);
}

bool SceneSerializer::loadCore(Scene& scene, const std::string& path, std::optional<uint32_t> forceSeed) {
    std::optional<json> parsed = parseSceneFile(path);
    if (!parsed) return false;
    const json& j = *parsed;

    scene.clear();

    const int version = j.value("version", -1);
    if (version != kSceneVersion) {
        Log::error("SceneSerializer",
                   std::format("Scene version mismatch in '{}': expected {}, got {}", path, kSceneVersion, version));
        return false;
    }

    std::mt19937 rng(resolveSeed(j, forceSeed));
    ResolveCtx ctx{rng, {}};

    replaceSceneRootIfProvided(j, scene);

    SpawnContext spawn{scene.getRegistry(), scene.getAnimationStore(), {}, {}, {}};

    loadSection(j, "Scene", scene.getSceneRoot(), ctx, spawn);
    ensureSceneDefaults(scene);
    loadSection(j, "Materials", scene.getMaterialsRoot(), ctx, spawn);
    loadSection(j, "Assets", scene.getAssetsRoot(), ctx, spawn);
    loadSection(j, "Objects", scene.getObjectsRoot(), ctx, spawn);

    resolveScenePaths(spawn.registry, std::filesystem::absolute(path).parent_path());
    resolveDeferredEntityFields(spawn.registry, spawn.deferredEntityFields, buildEntityNameMap(scene, spawn.registry));
    reloadMeshAssets(spawn.registry, scene);

    spawn.animStore.evaluate(0.0f);
    return true;
}

bool SceneSerializer::save(Scene& scene, const std::string& path) {
    json j;
    j["version"] = kSceneVersion;

    const std::filesystem::path sceneDirectory = std::filesystem::absolute(path).parent_path();

    ecs::Registry& reg = scene.getRegistry();
    const AnimationStore& animStore = scene.getAnimationStore();

    std::function<json(ecs::Entity)> saveNode;
    saveNode = [&](ecs::Entity e) -> json {
        json node = json::object();

        if (reg.has(e, ecs::Name)) node["name"] = reg.get(e, ecs::Name).get<std::string>("value");

        for (const ecs::ComponentType& type : ecs::ComponentType::all()) {
            if (!reg.has(e, type)) continue;
            if (type.getId() == "name") continue;
            if (type.getId() == "material") continue;
            if (type == ecs::Compositing) {
                node["compositing"] = serializeCompositingPasses(reg.get(e, type), sceneDirectory);
                continue;
            }
            node[type.getId()] = serializeComponent(reg.get(e, type), animStore, reg, sceneDirectory);
        }

        json childrenJson = json::array();
        for (const ecs::Entity& child : reg.getChildren(e)) {
            if (child == scene.getDefaultMaterial()) continue;
            if (child == scene.getDefaultMesh()) continue;
            childrenJson.push_back(saveNode(child));
        }
        if (!childrenJson.empty()) node["children"] = childrenJson;

        return node;
    };

    auto saveSection = [&](ecs::Entity root, ecs::Entity skip = {}) -> json {
        json arr = json::array();
        for (const ecs::Entity& child : reg.getChildren(root)) {
            if (child == skip) continue;
            arr.push_back(saveNode(child));
        }
        return arr;
    };

    j["Scene"] = saveSection(scene.getSceneRoot());
    j["Materials"] = saveSection(scene.getMaterialsRoot(), scene.getDefaultMaterial());
    j["Assets"] = saveSection(scene.getAssetsRoot(), scene.getDefaultMesh());
    j["Objects"] = saveSection(scene.getObjectsRoot());

    std::ofstream out(path);
    if (!out.is_open()) {
        Log::error("SceneSerializer", std::format("Failed to write scene: {}", path));
        return false;
    }
    out << prettifyJson(j);
    return true;
}
