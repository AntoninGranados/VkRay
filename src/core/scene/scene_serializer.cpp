#include "scene_serializer.hpp"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <random>
#include <unordered_map>
#include <utility>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include "core/core.hpp"
#include "core/ecs/systems/mesh_system.hpp"
#include "scene.hpp"
#include "utils/log.hpp"

static constexpr int SCENE_VERSION = 1;

namespace {

constexpr std::pair<const char*, LightMode> kLightModes[] = {
    {"day", LightMode::Day},
    {"sunset", LightMode::Sunset},
    {"night", LightMode::Night},
    {"empty", LightMode::Empty},
};
constexpr std::pair<const char*, Interpolation> kInterpolations[] = {
    {"linear", Interpolation::Linear},
    {"step", Interpolation::Step},
    {"cubic", Interpolation::Cubic},
    {"ease_in", Interpolation::EaseIn},
    {"ease_out", Interpolation::EaseOut},
    {"ease_in_out", Interpolation::EaseInOut},
};

template<typename T, size_t N>
T fromStr(const std::pair<const char*, T> (&table)[N], const std::string& s, T fallback) {
    for (const auto& [name, val] : table) if (s == name) return val;
    return fallback;
}
template<typename T, size_t N>
const char* toStr(const std::pair<const char*, T> (&table)[N], T e) {
    for (const auto& [name, val] : table) if (e == val) return name;
    std::unreachable();
}

} // namespace

json SceneSerializer::valueToJson(const FieldValue& f) {
    switch (f.getType()) {
        case FieldType::Bool:   return f.get<bool>();
        case FieldType::Int:
        case FieldType::Enum:   return f.get<int>();
        case FieldType::Float:  return f.get<float>();
        case FieldType::IVec2: { auto v = f.get<glm::ivec2>(); return json{v.x, v.y}; }
        case FieldType::IVec3: { auto v = f.get<glm::ivec3>(); return json{v.x, v.y, v.z}; }
        case FieldType::IVec4: { auto v = f.get<glm::ivec4>(); return json{v.x, v.y, v.z, v.w}; }
        case FieldType::Vec2:  { auto v = f.get<glm::vec2>(); return json{v.x, v.y}; }
        case FieldType::Vec3:  { auto v = f.get<glm::vec3>(); return json{v.x, v.y, v.z}; }
        case FieldType::Vec4:  { auto v = f.get<glm::vec4>(); return json{v.x, v.y, v.z, v.w}; }
        case FieldType::Quat:  { auto v = f.get<glm::quat>(); return json{v.x, v.y, v.z, v.w}; }
        case FieldType::Entity: return nullptr;
        case FieldType::String: return trimmed(f.get<std::string>());
        case FieldType::Path:   return f.get<std::filesystem::path>().string();
    }
    std::unreachable();
}

void SceneSerializer::applyField(const json& j, Field& f, const ResolveCtx& ctx) {
    switch (f.getType()) {
        case FieldType::Bool:   f.set<bool>(j.get<bool>()); break;
        case FieldType::Int:
        case FieldType::Enum:   f.set<int>(j.is_number_integer() ? j.get<int>() : static_cast<int>(std::round(resolveFloat(j, ctx)))); break;
        case FieldType::IVec2:  if (expectArray(j, 2, f.getId())) f.set<glm::ivec2>({j[0].get<int>(), j[1].get<int>()}); break;
        case FieldType::IVec3:  if (expectArray(j, 3, f.getId())) f.set<glm::ivec3>({j[0].get<int>(), j[1].get<int>(), j[2].get<int>()}); break;
        case FieldType::IVec4:  if (expectArray(j, 4, f.getId())) f.set<glm::ivec4>({j[0].get<int>(), j[1].get<int>(), j[2].get<int>(), j[3].get<int>()}); break;
        case FieldType::Float:  f.set<float>(resolveFloat(j, ctx)); break;
        case FieldType::Vec2:   f.set<glm::vec2>(resolveVec2(j, ctx)); break;
        case FieldType::Vec3:   f.set<glm::vec3>(resolveVec3(j, ctx)); break;
        case FieldType::Vec4:   if (expectArray(j, 4, f.getId())) f.set<glm::vec4>({resolveFloat(j[0], ctx), resolveFloat(j[1], ctx), resolveFloat(j[2], ctx), resolveFloat(j[3], ctx)}); break;
        case FieldType::Quat:   if (expectArray(j, 4, f.getId())) f.set<glm::quat>(glm::quat(j[3].get<float>(), j[0].get<float>(), j[1].get<float>(), j[2].get<float>())); break;
        case FieldType::Entity: break;
        case FieldType::String: f.set<std::string>(resolveTemplate(j.get<std::string>(), ctx)); break;
        case FieldType::Path:   f.set<std::filesystem::path>(j.get<std::string>()); break;
    }
}

FieldValue SceneSerializer::toFieldValue(const json& v, FieldType type) {
    switch (type) {
        case FieldType::Bool:  return FieldValue::make(v.get<bool>());
        case FieldType::Int:
        case FieldType::Enum:  return FieldValue::make(v.get<int>());
        case FieldType::Float: return FieldValue::make(v.get<float>());
        case FieldType::Vec2:  return FieldValue::make(glm::vec2{v[0].get<float>(), v[1].get<float>()});
        case FieldType::Vec3:  return FieldValue::make(glm::vec3{v[0].get<float>(), v[1].get<float>(), v[2].get<float>()});
        case FieldType::Vec4:  return FieldValue::make(glm::vec4{v[0].get<float>(), v[1].get<float>(), v[2].get<float>(), v[3].get<float>()});
        default:
            Log::error("SceneSerializer", std::format("Unsupported keyframe field type: {}", static_cast<int>(type)));
            return FieldValue::make(false);
    }
}

json SceneSerializer::serializeKeyframes(const std::map<int, Keyframe>& kfs) {
    json arr = json::array();
    for (const auto& [frame, kf] : kfs) {
        json kfj;
        kfj["frame"] = frame;
        kfj["value"] = valueToJson(kf.getValue());
        if (kf.getInterpolation() != Interpolation::Linear)
            kfj["ease"] = toStr(kInterpolations, kf.getInterpolation());
        arr.push_back(kfj);
    }
    return arr;
}

json SceneSerializer::serializeField(const Field& f, const std::map<int, Keyframe>& kfs) {
    if (!kfs.empty()) return json{{"anim", serializeKeyframes(kfs)}};
    return valueToJson(f);
}

void SceneSerializer::applyKeyframes(const json& anim, FieldType type, const std::string& fieldId, const std::function<void(int, FieldValue, Interpolation)>& insert) {
    for (const auto& kf : anim) {
        if (!kf.contains("frame") || !kf.contains("value")) {
            Log::error("SceneSerializer", std::format("Keyframe for '{}' missing 'frame' or 'value'", fieldId));
            continue;
        }
        const Interpolation interp = kf.contains("ease")
            ? fromStr(kInterpolations, kf["ease"].get<std::string>(), Interpolation::Linear)
            : Interpolation::Linear;
        insert(kf["frame"].get<int>(), toFieldValue(kf["value"], type), interp);
    }
}

json SceneSerializer::serializeComponent(const ecs::Component& comp, ecs::Entity e, const AnimationStore& animStore, const ecs::Registry& registry) {
    json j = json::object();
    for (const auto& f : comp.getFields()) {
        if (f.getType() == FieldType::Entity) {
            const ecs::Entity referenced = f.get<ecs::Entity>();
            if (referenced != ecs::Entity{} && registry.has(referenced, ecs::Name)) {
                const std::string name = registry.get(referenced, ecs::Name).get<std::string>("value");
                if (!name.empty()) j[f.getId()] = name;
            }
            continue;
        }
        j[f.getId()] = serializeField(f, animStore.keyframes(e, comp.getType(), f.getId()));
    }
    return j;
}

void SceneSerializer::applyComponent(const json& obj, ecs::Component& comp, ecs::Entity e, AnimationStore& animStore, const ResolveCtx& ctx) {
    for (auto& f : comp.getFields()) {
        if (!obj.contains(f.getId())) continue;
        if (f.getType() == FieldType::Entity) continue;
        const json& val = obj[f.getId()];
        if (val.is_object() && val.contains("anim") && val["anim"].is_array())
            applyKeyframes(val["anim"], f.getType(), f.getId(), [&](int frame, FieldValue value, Interpolation interp) {
                animStore.insert(e, comp.getType(), f.getId(), frame, std::move(value), interp);
            });
        else
            applyField(val, f, ctx);
    }
}

void SceneSerializer::spawnSpherical(const json& node, ecs::Entity e, ecs::Registry& registry, const ResolveCtx& ctx) {
    if (!node.contains("spherical")) return;
    const auto& sj = node["spherical"];
    const float radius = resolveFloat(sj["radius"], ctx);
    const float azDeg = resolveFloat(sj["azimuth"], ctx);
    const float elDeg = resolveFloat(sj["elevation"], ctx);
    const glm::vec3 target = sj.contains("target") ? resolveVec3(sj["target"], ctx) : glm::vec3{0, 0, 0};

    const float az = glm::radians(azDeg);
    const float el = glm::radians(elDeg);
    const glm::vec3 pos = target + radius * glm::vec3(std::cos(el) * std::sin(az), std::sin(el), std::cos(el) * std::cos(az));
    const glm::vec3 dir = glm::normalize(target - pos);

    auto ttype = ecs::ComponentType::find("transform");
    if (!ttype || !registry.add(e, ttype->get())) return;
    auto& t = registry.get(e, ttype->get());
    t.set<glm::vec3>("position", pos);
    t.set<glm::vec3>("rotation", {glm::degrees(std::asin(glm::clamp(dir.y, -1.0f, 1.0f))), glm::degrees(std::atan2(dir.x, -dir.z)), 0.0f});
}

std::string SceneSerializer::inlineJson(const json& j) {
    if (j.is_object()) {
        std::string s = "{ ";
        bool first = true;
        for (const auto& [k, v] : j.items()) {
            if (!first) s += ", ";
            s += '"' + k + "\": " + inlineJson(v);
            first = false;
        }
        return s + " }";
    }
    if (j.is_array()) {
        std::string s = "[";
        for (size_t i = 0; i < j.size(); i++) {
            if (i > 0) s += ", ";
            s += inlineJson(j[i]);
        }
        return s + "]";
    }
    return j.dump();
}

std::string SceneSerializer::prettify(const json& j, int indent) {
    auto isVector = [](const json& v) { return v.is_array() && std::all_of(v.begin(), v.end(), [](const json& e){ return e.is_number(); }); };
    auto isExpression = [](const json& v) { return v.is_object() && v.size() == 1 && (v.contains("rand") || v.contains("lerp")); };
    auto isKeyframe = [](const json& v) {
        if (!v.is_object()) return false;
        const size_t n = v.size();
        return (n == 2 || n == 3) && v.contains("frame") && v.contains("value") && (n == 2 || v.contains("ease"));
    };
    if (isVector(j) || isExpression(j) || isKeyframe(j)) return inlineJson(j);

    if (j.is_object()) {
        std::string inlined = inlineJson(j);
        if (inlined.size() <= 60) return inlined;
    }

    const std::string pad(indent * 4, ' ');
    const std::string inner((indent + 1) * 4, ' ');

    if (j.is_array()) {
        std::string s = "[\n";
        for (size_t i = 0; i < j.size(); i++) {
            s += inner + prettify(j[i], indent + 1);
            if (i + 1 < j.size()) s += ',';
            s += '\n';
        }
        return s + pad + "]";
    }
    if (j.is_object()) {
        std::string s = "{\n";
        size_t i = 0;
        for (const auto& [k, v] : j.items()) {
            s += inner + '"' + k + "\": " + prettify(v, indent + 1);
            if (++i < j.size()) s += ',';
            s += '\n';
        }
        return s + pad + "}";
    }
    return j.dump();
}

bool SceneSerializer::load(Scene& scene, LightMode& lightMode, const std::string& path, std::optional<uint32_t> forceSeed) {
    std::ifstream file(path);
    if (!file.is_open()) {
        Log::error("SceneSerializer", std::format("Cannot open scene: {}", path));
        return false;
    }

    json j;
    try { j = json::parse(file, nullptr, true, true); }
    catch (const json::parse_error& e) {
        Log::error("SceneSerializer", std::format("Scene parse error: {}", e.what()));
        return false;
    }

    Core::getEngine().waitIdle();
    scene.clear();

    const int version = j.value("version", -1);
    if (version != SCENE_VERSION) {
        Log::error("SceneSerializer", std::format("Scene version mismatch in '{}': expected {}, got {}", path, SCENE_VERSION, version));
        return false;
    }

    const uint32_t seed = forceSeed.value_or(
        j.contains("seed") ? j["seed"].get<uint32_t>() : static_cast<uint32_t>(std::random_device{}())
    );
    std::mt19937 rng(seed);
    ResolveCtx ctx{ rng, {} };

    if (j.contains("light"))
        lightMode = fromStr(kLightModes, j["light"].get<std::string>(), LightMode::Day);

    ecs::Registry& registry = scene.getRegistry();
    AnimationStore& animStore = scene.getAnimationStore();

    struct DeferredEntityField {
        ecs::Entity entity;
        const ecs::ComponentType* componentType;
        std::string fieldId;
        std::string entityName;
    };
    std::vector<DeferredEntityField> deferredEntityFields;

    auto spawnComponents = [&](const json& node, ecs::Entity e, const ResolveCtx& spawnCtx) {
        for (const auto& [key, value] : node.items()) {
            if (key == "name" || key == "children" || key == "repeat" || key == "grid" || key == "spherical") continue;
            auto type = ecs::ComponentType::find(key);
            if (!type) { Log::warn("SceneSerializer", std::format("Unknown component '{}'", key)); continue; }
            if (registry.add(e, type->get()) && value.is_object()) {
                for (const auto& field : registry.get(e, type->get()).getFields()) {
                    if (field.getType() != FieldType::Entity) continue;
                    if (!value.contains(field.getId()) || !value[field.getId()].is_string()) continue;
                    deferredEntityFields.push_back({e, &type->get(), field.getId(), resolveTemplate(value[field.getId()].get<std::string>(), spawnCtx)});
                }
                applyComponent(value, registry.get(e, type->get()), e, animStore, spawnCtx);
            }
        }
    };

    std::function<void(const json&, ecs::Entity, const ResolveCtx&)> loadNode;
    loadNode = [&](const json& node, ecs::Entity parent, const ResolveCtx& nodeCtx) {
        auto spawnOne = [&](const ResolveCtx& spawnCtx) {
            ecs::Entity e = registry.createEntity(parent);

            if (node.contains("name") && node["name"].is_string()) {
                const std::string name = resolveTemplate(node["name"].get<std::string>(), spawnCtx);
                registry.add(e, ecs::Name);
                registry.get(e, ecs::Name).set<std::string>("value", name);
            }

            spawnComponents(node, e, spawnCtx);
            spawnSpherical(node, e, registry, spawnCtx);

            if (node.contains("children") && node["children"].is_array()) {
                for (const auto& child : node["children"]) {
                    if (child.is_object()) loadNode(child, e, spawnCtx);
                }
            }
        };

        if (node.contains("repeat")) {
            const int count = node["repeat"].value("count", 1);
            for (int n = 0; n < count; n++)
                spawnOne(ResolveCtx{ nodeCtx.rng, {{"n", {n, count}}} });
        } else if (node.contains("grid")) {
            const auto& g = node["grid"];
            const int rows = g.value("rows", 1);
            const int cols = g.value("cols", 1);
            for (int r = 0; r < rows; r++)
                for (int c = 0; c < cols; c++)
                    spawnOne(ResolveCtx{ nodeCtx.rng, {{"row", {r, rows}}, {"col", {c, cols}}, {"n", {r * cols + c, rows * cols}}} });
        } else {
            spawnOne(nodeCtx);
        }
    };

    auto loadSection = [&](const std::string& key, ecs::Entity root) {
        if (!j.contains(key) || !j[key].is_array()) return;
        for (const auto& child : j[key])
            if (child.is_object()) loadNode(child, root, ctx);
    };

    loadSection("Materials", scene.getMaterialsRoot());
    loadSection("Assets", scene.getAssetsRoot());
    loadSection("Objects", scene.getObjectsRoot());

    std::unordered_map<std::string, ecs::Entity> entityByName;
    for (const ecs::Entity& entity : scene.getChildren(scene.getMaterialsRoot())) {
        const std::string name = registry.get(entity, ecs::Name).get<std::string>("value");
        if (!name.empty()) entityByName[name] = entity;
    }
    for (const ecs::Entity& entity : scene.getChildren(scene.getAssetsRoot())) {
        const std::string name = registry.get(entity, ecs::Name).get<std::string>("value");
        if (!name.empty()) entityByName[name] = entity;
    }

    for (const DeferredEntityField& deferred : deferredEntityFields) {
        const auto found = entityByName.find(deferred.entityName);
        if (found != entityByName.end())
            registry.get(deferred.entity, *deferred.componentType).set<ecs::Entity>(deferred.fieldId, found->second);
        else
            Log::warn("SceneSerializer", std::format("Entity '{}' not found for field '{}'", deferred.entityName, deferred.fieldId));
    }

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

        if (registry.has(entity, ecs::MeshSimplify)) {
            const float ratio = registry.get(entity, ecs::MeshSimplify).get<float>("ratio");
            ecs::requestMeshSimplify(registry, entity, ratio);
        }
    }

    animStore.evaluate(registry, 0.0f);
    return true;
}

bool SceneSerializer::save(Scene& scene, LightMode lightMode, const std::string& path) {
    json j;
    j["version"] = SCENE_VERSION;
    j["light"] = toStr(kLightModes, lightMode);

    ecs::Registry& reg = scene.getRegistry();
    const AnimationStore& animStore = scene.getAnimationStore();

    std::function<json(ecs::Entity)> saveNode;
    saveNode = [&](ecs::Entity e) -> json {
        json node = json::object();

        if (reg.has(e, ecs::Name))
            node["name"] = reg.get(e, ecs::Name).get<std::string>("value");

        for (const ecs::ComponentType& type : ecs::ComponentType::all()) {
            if (!reg.has(e, type)) continue;
            if (type.getId() == "name") continue;
            if (type.getId() == "material") continue;
            node[type.getId()] = serializeComponent(reg.get(e, type), e, animStore, reg);
        }

        json childrenJson = json::array();
        for (const ecs::Entity& child : reg.getChildren(e)) {
            if (child == scene.getDefaultMaterial()) continue;
            if (child == scene.getDefaultMeshAsset()) continue;
            childrenJson.push_back(saveNode(child));
        }
        if (!childrenJson.empty())
            node["children"] = childrenJson;

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

    j["Materials"] = saveSection(scene.getMaterialsRoot(), scene.getDefaultMaterial());
    j["Assets"] = saveSection(scene.getAssetsRoot(), scene.getDefaultMeshAsset());
    j["Objects"] = saveSection(scene.getObjectsRoot());

    std::ofstream out(path);
    if (!out.is_open()) {
        Log::error("SceneSerializer", std::format("Failed to write scene: {}", path));
        return false;
    }
    out << prettify(j);
    return true;
}
