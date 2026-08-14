#pragma once

#include <functional>
#include <map>
#include <optional>
#include <string>

#include "nlohmann/json.hpp"

#include "core/animation/animation_store.hpp"
#include "core/scene/object.hpp"
#include "utils/json_resolve.hpp"

using json = nlohmann::ordered_json;

class Scene;
enum LightMode : int;

class SceneSerializer {
public:
    static bool load(Scene& scene, LightMode& lightMode, const std::string& path, std::optional<uint32_t> seed = std::nullopt);
    static bool save(Scene& scene, LightMode lightMode, const std::string& path);

private:
    static json       fieldToJson(const Field& f);
    static void       applyField(const json& j, Field& f, const ResolveCtx& ctx);
    static json       fieldValueToJson(const FieldValue& fv);
    static FieldValue toFieldValue(const json& v, FieldType type);

    static json    serializeKeyframes(const std::map<int, Keyframe>& kfs);
    static json    serializeField(const Field& f, const std::map<int, Keyframe>& kfs);
    static void    applyKeyframes(const json& anim, FieldType type, const std::string& fieldId, const std::function<void(int, FieldValue, Interpolation)>& insert);

    static json    serializeComponentWithAnim(const ecs::Component& comp, ecs::Entity e, const AnimationStore& animStore);
    static void    applyComponent(const json& obj, ecs::Component& comp, ecs::Entity e, AnimationStore& animStore, const ResolveCtx& ctx);

    static void    spawnMesh(const json& ej, ecs::Entity e, Scene& scene, ecs::Registry& registry);
    static void    spawnSpherical(const json& ej, ecs::Entity e, ecs::Registry& registry, const ResolveCtx& ctx);

    static std::string inlineJson(const json& j);
    static std::string prettify(const json& j, int indent = 0);
};
