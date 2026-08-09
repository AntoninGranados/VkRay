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
#include "scene.hpp"
#include "utils/log.hpp"

static constexpr int SCENE_VERSION = 1;

namespace {

constexpr std::pair<const char*, MaterialType> kMaterialTypes[] = {
    {"principled", MaterialType::Principled},
    {"emissive", MaterialType::Emissive},
    {"lambertian", MaterialType::Lambertian},
    {"ggx_metal", MaterialType::GgxMetal},
    {"ggx_glossy", MaterialType::GgxGlossy},
    {"dielectric", MaterialType::Dielectric},
    {"volume", MaterialType::Volume},
    {"programmable", MaterialType::Programmable},
};
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
T fromStr(const std::pair<const char*, T> (&table)[N], const std::string& s) {
    for (const auto& [name, val] : table) if (s == name) return val;
    std::unreachable();
}
template<typename T, size_t N>
const char* toStr(const std::pair<const char*, T> (&table)[N], T e) {
    for (const auto& [name, val] : table) if (e == val) return name;
    std::unreachable();
}

} // namespace

json SceneSerializer::fieldToJson(const Field& f) {
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
        case FieldType::String: return trimmed(f.get<std::string>());
        case FieldType::Path:   return f.get<std::filesystem::path>().string();
    }
    std::unreachable();
}

void SceneSerializer::applyField(const json& j, Field& f, const ResolveCtx& ctx) {
    switch (f.getType()) {
        case FieldType::Bool:   f.set<bool>(j.get<bool>()); break;
        case FieldType::Int:
        case FieldType::Enum:   f.set<int>(j.get<int>()); break;
        case FieldType::IVec2:  if (expectArray(j, 2, f.getId())) f.set<glm::ivec2>({j[0].get<int>(), j[1].get<int>()}); break;
        case FieldType::IVec3:  if (expectArray(j, 3, f.getId())) f.set<glm::ivec3>({j[0].get<int>(), j[1].get<int>(), j[2].get<int>()}); break;
        case FieldType::IVec4:  if (expectArray(j, 4, f.getId())) f.set<glm::ivec4>({j[0].get<int>(), j[1].get<int>(), j[2].get<int>(), j[3].get<int>()}); break;
        case FieldType::Float:  f.set<float>(resolveFloat(j, ctx)); break;
        case FieldType::Vec2:   f.set<glm::vec2>(resolveVec2(j, ctx)); break;
        case FieldType::Vec3:   f.set<glm::vec3>(resolveVec3(j, ctx)); break;
        case FieldType::Vec4:   if (expectArray(j, 4, f.getId())) f.set<glm::vec4>({resolveFloat(j[0], ctx), resolveFloat(j[1], ctx), resolveFloat(j[2], ctx), resolveFloat(j[3], ctx)}); break;
        case FieldType::Quat:   if (expectArray(j, 4, f.getId())) f.set<glm::quat>(glm::quat(j[3].get<float>(), j[0].get<float>(), j[1].get<float>(), j[2].get<float>())); break;
        case FieldType::String: f.set<std::string>(resolveTemplate(j.get<std::string>(), ctx)); break;
        case FieldType::Path:   f.set<std::filesystem::path>(j.get<std::string>()); break;
    }
}

json SceneSerializer::fieldValueToJson(const FieldValue& fv) {
    switch (fv.getType()) {
        case FieldType::Bool:  return fv.get<bool>();
        case FieldType::Int:
        case FieldType::Enum:  return fv.get<int>();
        case FieldType::Float: return fv.get<float>();
        case FieldType::Vec2:  { auto v = fv.get<glm::vec2>(); return json{v.x, v.y}; }
        case FieldType::Vec3:  { auto v = fv.get<glm::vec3>(); return json{v.x, v.y, v.z}; }
        case FieldType::Vec4:  { auto v = fv.get<glm::vec4>(); return json{v.x, v.y, v.z, v.w}; }
        default:
            Log::error("SceneSerializer", std::format("Unsupported keyframe field type: {}", static_cast<int>(fv.getType())));
            return nullptr;
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
        kfj["value"] = fieldValueToJson(kf.getValue());
        if (kf.getInterpolation() != Interpolation::Linear)
            kfj["ease"] = toStr(kInterpolations, kf.getInterpolation());
        arr.push_back(kfj);
    }
    return arr;
}

json SceneSerializer::serializeField(const Field& f, const std::map<int, Keyframe>& kfs) {
    if (!kfs.empty()) return json{{"anim", serializeKeyframes(kfs)}};
    return fieldToJson(f);
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

json SceneSerializer::serializeComponentWithAnim(const ecs::Component& comp, ecs::Entity e, const AnimationStore& animStore) {
    json j = json::object();
    for (const auto& f : comp.getFields()) {
        if (f.isPrivate()) continue;
        j[f.getId()] = serializeField(f, animStore.keyframes(e, comp.getType(), f.getId()));
    }
    return j;
}

void SceneSerializer::applyComponent(const json& obj, ecs::Component& comp, ecs::Entity e, AnimationStore& animStore, const ResolveCtx& ctx) {
    for (auto& f : comp.getFields()) {
        if (f.isPrivate() || !obj.contains(f.getId())) continue;
        const json& val = obj[f.getId()];
        if (val.is_object() && val.contains("anim") && val["anim"].is_array())
            applyKeyframes(val["anim"], f.getType(), f.getId(), [&](int frame, FieldValue value, Interpolation interp) {
                animStore.insert(e, comp.getType(), f.getId(), frame, std::move(value), interp);
            });
        else
            applyField(val, f, ctx);
    }
}

json SceneSerializer::serializeMaterial(const Material& m, MaterialHandle h, const AnimationStore& animStore) {
    json mj;
    mj["name"] = trimmed(m.getName());
    mj["bsdf"] = toStr(kMaterialTypes, m.getType());
    for (const auto& fieldId : Material::fieldsForType(m.getType())) {
        const Field& f = m.getField(fieldId);
        mj[fieldId] = serializeField(f, animStore.keyframes(h, fieldId));
    }
    return mj;
}

void SceneSerializer::applyMaterial(const json& m, MaterialHandle handle, AnimationStore& animStore, const ResolveCtx& ctx) {
    Material& mat = Core::getScene().getMaterials()[static_cast<size_t>(handle)];
    for (auto& f : mat.getFields()) {
        if (!m.contains(f.getId())) continue;
        const json& val = m[f.getId()];
        if (val.is_object() && val.contains("anim") && val["anim"].is_array())
            applyKeyframes(val["anim"], f.getType(), f.getId(), [&](int frame, FieldValue value, Interpolation interp) {
                animStore.insert(handle, f.getId(), frame, std::move(value), interp);
            });
        else
            applyField(val, f, ctx);
    }
}

void SceneSerializer::spawnMesh(const json& ej, ecs::Entity e, Scene& scene, ecs::Registry& registry) {
    if (!ej.contains("mesh") || !ej["mesh"].is_object()) return;
    const auto& mj = ej["mesh"];
    const std::string meshPath = mj.value("path", "");
    if (meshPath.empty()) return;
    const MeshHandle handle = static_cast<MeshHandle>(scene.getMeshAssets().size());
    scene.getMeshAssets().emplace_back(MeshAsset::nameFromPath(meshPath));
    MeshAsset& asset = scene.getMeshAssets().back();
    asset.setSmoothShading(mj.value("smooth", false));
    if (asset.loadFromObj(meshPath)) {
        if (registry.add(e, ecs::MeshRef))
            registry.get(e, ecs::MeshRef).set<int>("handle", handle);
    } else {
        scene.getMeshAssets().pop_back();
        Log::error("SceneSerializer", std::format("Failed to load mesh: {}", meshPath));
    }
}

void SceneSerializer::spawnMaterialRef(const json& ej, ecs::Entity e, ecs::Registry& registry,
                                       const std::unordered_map<std::string, MaterialHandle>& matMap,
                                       const ResolveCtx& ctx) {
    if (!ej.contains("material") || !ej["material"].is_string()) return;
    const std::string matName = resolveTemplate(ej["material"].get<std::string>(), ctx);
    const auto it = matMap.find(matName);
    if (it != matMap.end() && registry.add(e, ecs::MaterialRef))
        registry.get(e, ecs::MaterialRef).set<int>("handle", it->second);
    else
        Log::error("SceneSerializer", std::format("Material '{}' not found", matName));
}

void SceneSerializer::spawnSpherical(const json& ej, ecs::Entity e, ecs::Registry& registry, const ResolveCtx& ctx) {
    if (!ej.contains("spherical")) return;
    const auto& sj = ej["spherical"];
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
        lightMode = fromStr(kLightModes, j["light"].get<std::string>());

    std::unordered_map<std::string, MaterialHandle> matMap;
    if (j.contains("materials")) {
        for (const auto& m : j["materials"]) {
            auto push = [&](const ResolveCtx& matCtx) {
                const std::string name = resolveTemplate(m.value("name", "Unnamed"), matCtx);
                const MaterialType type = fromStr(kMaterialTypes, m.value("bsdf", m.value("type", "lambertian")));
                const MaterialHandle h = scene.pushMaterial(Material::make(type, name));
                matMap[name] = h;
                applyMaterial(m, h, scene.getAnimationStore(), matCtx);
            };
            if (m.contains("repeat")) {
                const int count = m["repeat"].value("count", 1);
                for (int n = 0; n < count; n++)
                    push(ResolveCtx{ rng, {{"n", {n, count}}} });
            } else if (m.contains("grid")) {
                const auto& g = m["grid"];
                const int rows = g.value("rows", 1);
                const int cols = g.value("cols", 1);
                for (int r = 0; r < rows; r++)
                    for (int c = 0; c < cols; c++)
                        push(ResolveCtx{ rng, {{"row", {r, rows}}, {"col", {c, cols}}} });
            } else {
                push(ctx);
            }
        }
    }

    if (!j.contains("entities")) return true;

    ecs::Registry& registry = scene.getRegistry();
    auto spawnEntity = [&](const json& ej, const ResolveCtx& spawnCtx) {
        ecs::Entity e = registry.createEntity();
        scene.getEntities().push_back(e);
        spawnMesh(ej, e, scene, registry);
        spawnMaterialRef(ej, e, registry, matMap, spawnCtx);
        for (const auto& [key, value] : ej.items()) {
            if (key == "material" || key == "mesh" || key == "repeat" || key == "grid" || key == "spherical") continue;
            auto type = ecs::ComponentType::find(key);
            if (!type) { Log::warn("SceneSerializer", std::format("Unknown component '{}'", key)); continue; }
            if (registry.add(e, type->get()) && value.is_object())
                applyComponent(value, registry.get(e, type->get()), e, scene.getAnimationStore(), spawnCtx);
        }
        spawnSpherical(ej, e, registry, spawnCtx);
    };

    for (const auto& ej : j["entities"]) {
        if (!ej.is_object()) continue;
        if (ej.contains("repeat")) {
            const int count = ej["repeat"].value("count", 1);
            for (int n = 0; n < count; n++)
                spawnEntity(ej, ResolveCtx{ rng, {{"n", {n, count}}} });
        } else if (ej.contains("grid")) {
            const auto& g = ej["grid"];
            const int rows = g.value("rows", 1);
            const int cols = g.value("cols", 1);
            for (int r = 0; r < rows; r++)
                for (int c = 0; c < cols; c++)
                    spawnEntity(ej, ResolveCtx{ rng, {{"row", {r, rows}}, {"col", {c, cols}}} });
        } else {
            spawnEntity(ej, ctx);
        }
    }

    AnimationStore& animStore = scene.getAnimationStore();
    animStore.evaluate(registry, 0.0f);
    animStore.evaluate(scene.getMaterials(), 0.0f);
    return true;
}

bool SceneSerializer::save(Scene& scene, LightMode lightMode, const std::string& path) {
    json j;
    j["version"] = SCENE_VERSION;
    j["light"] = toStr(kLightModes, lightMode);

    const auto& mats = scene.getMaterials();
    const AnimationStore& animStore = scene.getAnimationStore();
    json matsJson = json::array();
    for (size_t i = 1; i < mats.size(); i++)
        matsJson.push_back(serializeMaterial(mats[i], static_cast<MaterialHandle>(i), animStore));
    j["materials"] = matsJson;

    ecs::Registry& reg = scene.getRegistry();
    const auto& assets = scene.getMeshAssets();
    json entitiesJson = json::array();
    for (const ecs::Entity& e : scene.getEntities()) {
        json ej = json::object();
        for (const ecs::ComponentType& type : ecs::ComponentType::all()) {
            if (!reg.has(e, type)) continue;
            const ecs::Component& comp = reg.get(e, type);
            if (type.getId() == "material") {
                const int handle = comp.get<int>("handle");
                if (handle > 0 && handle < (int)mats.size())
                    ej["material"] = trimmed(mats[handle].getName());
                continue;
            }
            if (type.getId() == "mesh") {
                const int handle = comp.get<int>("handle");
                if (handle >= 0 && handle < (int)assets.size() && !assets[handle].getPath().empty())
                    ej["mesh"] = json{{"path", assets[handle].getPath()}, {"smooth", assets[handle].getSmoothShading()}};
                continue;
            }
            ej[type.getId()] = serializeComponentWithAnim(comp, e, animStore);
        }
        entitiesJson.push_back(ej);
    }
    j["entities"] = entitiesJson;

    std::ofstream out(path);
    if (!out.is_open()) {
        Log::error("SceneSerializer", std::format("Failed to write scene: {}", path));
        return false;
    }
    out << prettify(j);
    return true;
}
