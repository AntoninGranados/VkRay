#pragma once

#include <map>
#include <optional>
#include <string>
#include <unordered_map>

#include "nlohmann/json.hpp"

#include "core/animation/animation_store.hpp"
#include "utils/json_resolve.hpp"

using json = nlohmann::ordered_json;

class Scene;
enum LightMode : int;

class SceneSerializer {
public:
    // TODO: remove support for in object material definition and allow for repeat/grid in the material definition
    static bool load(Scene& scene, LightMode& lightMode, const std::string& path, std::optional<uint32_t> seed = std::nullopt);
    static bool save(Scene& scene, LightMode lightMode, const std::string& path);

private:
    static void applyFieldFromJson(Field& f, const json& j, const ResolveCtx& ctx);
    static json fieldToJson(const Field& f);
    static FieldValue jsonToFieldValue(FieldType type, const json& v);
    static json fieldValueToJson(const FieldValue& fv);
    static std::string inlineJson(const json& j);
    static std::string prettify(const json& j, int indent = 0);
    static json serializeKeyframes(const std::map<int, Keyframe>& kfs);
    static void applyComponentFromJson(ecs::Component& comp, const json& obj, const ResolveCtx& ctx);
    static void applyAnimFromJson(ecs::Component& comp, const json& obj, ecs::Entity e, AnimationStore& animStore);
    static void applyMatAnimFromJson(const json& m, MaterialHandle handle, AnimationStore& animStore);
    static json serializeComponentWithAnim(const ecs::Component& comp, ecs::Entity e, const AnimationStore& animStore);
    static Material parseMaterial(const json& m, const ResolveCtx& ctx, const std::string& fallbackName = "Unnamed");
    static json serializeMaterial(const Material& m, MaterialHandle h, const AnimationStore& animStore);
    static void spawnMesh(const json& ej, ecs::Entity e, Scene& scene, ecs::Registry& registry);
    static void spawnMaterialRef(const json& ej, ecs::Entity e, ecs::Registry& registry, const std::unordered_map<std::string, MaterialHandle>& matMap, const ResolveCtx& ctx);
    static void spawnSpherical(const json& ej, ecs::Entity e, ecs::Registry& registry, const ResolveCtx& ctx);
    static void initCameraFromEntities(Scene& scene, ecs::Registry& registry);
};
