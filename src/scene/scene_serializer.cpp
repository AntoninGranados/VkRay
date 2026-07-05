#include "scene_serializer.hpp"

#include <fstream>
#include <optional>
#include <print>
#include <random>
#include <unordered_map>
#include <utility>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include "nlohmann/json.hpp"

#include "scene.hpp"
#include "scene/object/material.hpp"
#include "utils/json_resolve.hpp"

using json = nlohmann::ordered_json;

static constexpr int SCENE_VERSION = 1;

// Fixes an issue when writing a std::string created from a C string to a JSON file
static std::string trimmed(const std::string& s) {
    const auto pos = s.find('\0');
    return pos != std::string::npos ? s.substr(0, pos) : s;
}

static bool isVector(const json& j) {
    return j.is_array() && std::all_of(j.begin(), j.end(), [](const json& v){ return v.is_number(); });
}

static bool isExpression(const json& j) {
    return j.is_object() && j.size() == 1 && (j.contains("rand") || j.contains("lerp"));
}

static std::string inlineJson(const json& j) {
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

static std::string prettify(const json& j, int indent = 0) {
    if (isVector(j) || isExpression(j)) return inlineJson(j);

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

static json fromVec3(glm::vec3 v) { return { v.x, v.y, v.z }; }
static json fromVec2(glm::vec2 v) { return { v.x, v.y }; }

static MaterialType parseMaterialType(const std::string& s) {
    if (s == "principled")   return MaterialType::Principled;
    if (s == "emissive")     return MaterialType::Emissive;
    if (s == "lambertian")   return MaterialType::Lambertian;
    if (s == "ggx_metal")    return MaterialType::GgxMetal;
    if (s == "ggx_glossy")   return MaterialType::GgxGlossy;
    if (s == "dielectric")   return MaterialType::Dielectric;
    if (s == "volume")       return MaterialType::Volume;
    if (s == "programmable") return MaterialType::Programmable;
    std::println(stderr, "[WARNING] Unknown material type '{}', defaulting to lambertian", s);
    return MaterialType::Lambertian;
}

static std::string toString(MaterialType t) {
    switch (t) {
        case MaterialType::Principled:   return "principled";
        case MaterialType::Emissive:     return "emissive";
        case MaterialType::Lambertian:   return "lambertian";
        case MaterialType::GgxMetal:     return "ggx_metal";
        case MaterialType::GgxGlossy:    return "ggx_glossy";
        case MaterialType::Dielectric:   return "dielectric";
        case MaterialType::Volume:       return "volume";
        case MaterialType::Programmable: return "programmable";
    }
    std::unreachable();
}

static LightMode parseLightMode(const std::string& s) {
    if (s == "day")    return LightMode::Day;
    if (s == "sunset") return LightMode::Sunset;
    if (s == "night")  return LightMode::Night;
    if (s == "empty")  return LightMode::Empty;
    std::println(stderr, "[WARNING] Unknown light mode '{}', defaulting to day", s);
    return LightMode::Day;
}

static std::string toString(LightMode m) {
    switch (m) {
        case LightMode::Day:    return "day";
        case LightMode::Sunset: return "sunset";
        case LightMode::Night:  return "night";
        case LightMode::Empty:  return "empty";
    }
    std::unreachable();
}

static Material parseMaterial(const json& m, const ResolveCtx& ctx, const std::string& fallbackName = "Unnamed") {
    Material mat{};
    mat.name             = resolveTemplate(m.value("name", fallbackName), ctx);
    mat.type             = parseMaterialType(m.value("type", "lambertian"));
    if (m.contains("albedo")) mat.albedo = resolveVec3(m["albedo"], ctx);
    mat.roughness        = m.contains("roughness")        ? resolveFloat(m["roughness"],        ctx) : 0.0f;
    mat.metalness        = m.contains("metalness")        ? resolveFloat(m["metalness"],        ctx) : 0.0f;
    mat.ior              = m.contains("ior")              ? resolveFloat(m["ior"],              ctx) : 0.0f;
    mat.transmission     = m.contains("transmission")     ? resolveFloat(m["transmission"],     ctx) : 0.0f;
    mat.emissionStrength = m.contains("emission_strength") ? resolveFloat(m["emission_strength"], ctx) : 0.0f;
    mat.density          = m.contains("density")          ? resolveFloat(m["density"],          ctx) : 1.0f;
    mat.anisotropic      = m.contains("anisotropic")      ? resolveFloat(m["anisotropic"],      ctx) : 0.0f;
    return mat;
}

static MaterialHandle resolveMaterial(
    const json& matField,
    const std::unordered_map<std::string, MaterialHandle>& matMap,
    Scene& scene,
    const ResolveCtx& ctx,
    const std::string& fallbackName)
{
    if (matField.is_string()) {
        const std::string name = resolveTemplate(matField.get<std::string>(), ctx);
        auto it = matMap.find(name);
        if (it == matMap.end()) {
            std::println(stderr, "[ERROR] Unknown material '{}'", name);
            return 0;
        }
        return it->second;
    }
    if (matField.is_object())
        return scene.pushMaterial(parseMaterial(matField, ctx, fallbackName));
    return 0;
}

static void spawnOne(
    const json& obj,
    Scene& scene,
    const std::unordered_map<std::string, MaterialHandle>& matMap,
    const ResolveCtx& ctx,
    const glm::vec3& posOffset = glm::vec3(0.0f))
{
    const std::string name = resolveTemplate(obj.value("name", "Object"), ctx);
    const std::string type = obj.value("type", "");

    MaterialHandle mat = 0;
    if (obj.contains("material"))
        mat = resolveMaterial(obj["material"], matMap, scene, ctx, name + "_mat");

    if (type == "sphere") {
        const glm::vec3 center = (obj.contains("center") ? resolveVec3(obj["center"], ctx) : glm::vec3(0.0f)) + posOffset;
        const float radius = obj.contains("radius") ? resolveFloat(obj["radius"], ctx) : 1.0f;
        scene.pushSphere(name, center, radius, mat);

    } else if (type == "plane") {
        const glm::vec3 point  = (obj.contains("point")  ? resolveVec3(obj["point"],  ctx) : glm::vec3(0.0f)) + posOffset;
        const glm::vec3 normal =  obj.contains("normal") ? resolveVec3(obj["normal"], ctx) : glm::vec3(0,1,0);
        scene.pushPlane(name, point, normal, mat);

    } else if (type == "box") {
        const glm::vec3 mn = (obj.contains("min") ? resolveVec3(obj["min"], ctx) : glm::vec3(-1.0f)) + posOffset;
        const glm::vec3 mx = (obj.contains("max") ? resolveVec3(obj["max"], ctx) : glm::vec3( 1.0f)) + posOffset;
        scene.pushBox(name, mn, mx, mat);

    } else if (type == "quad") {
        const glm::vec3 center = (obj.contains("center") ? resolveVec3(obj["center"], ctx) : glm::vec3(0.0f)) + posOffset;
        const glm::vec3 normal =  obj.contains("normal") ? resolveVec3(obj["normal"], ctx) : glm::vec3(0,1,0);
        const glm::vec2 scale  =  obj.contains("scale")  ? resolveVec2(obj["scale"],  ctx) : glm::vec2(1.0f);
        const float rotation   =  obj.contains("rotation") ? resolveFloat(obj["rotation"], ctx) : 0.0f;
        scene.pushQuad(name, center, normal, scale, rotation, mat);

    } else if (type == "mesh") {
        const std::string meshPath = obj.value("path", "");
        if (meshPath.empty()) { std::println(stderr, "[ERROR] '{}': missing 'path'", name); return; }
        const glm::vec3 pos    = (obj.contains("position") ? resolveVec3(obj["position"], ctx) : glm::vec3(0.0f)) + posOffset;
        const glm::vec3 rotDeg =  obj.contains("rotation") ? resolveVec3(obj["rotation"], ctx) : glm::vec3(0.0f);
        const glm::vec3 scale  =  obj.contains("scale")    ? resolveVec3(obj["scale"],    ctx) : glm::vec3(1.0f);
        const bool smooth      =  obj.value("smooth", false);
        glm::mat4 t = glm::translate(glm::mat4(1.0f), pos);
        t = t * glm::toMat4(glm::quat(glm::radians(rotDeg)));
        t = glm::scale(t, scale);
        scene.pushMesh(name, meshPath, t, mat, smooth);

    } else {
        std::println(stderr, "[ERROR] Unknown type '{}'", type);
        return;
    }

    const auto& entities = scene.getEntities();
    if (entities.empty()) return;
    const ecs::Entity last = entities.back();
    auto& reg = scene.getRegistry();

    if (obj.contains("collider")) {
        const auto& c = obj["collider"];
        ecs::Collider collider{};
        if (c.is_object()) {
            if (c.contains("restitution")) collider.restitution = resolveFloat(c["restitution"], ctx);
            if (c.contains("friction"))    collider.friction    = resolveFloat(c["friction"],    ctx);
        }
        if (!reg.has<ecs::Collider>(last)) reg.add<ecs::Collider>(last, collider);
    }

    if (obj.contains("rigid_body")) {
        const auto& rb = obj["rigid_body"];
        ecs::RigidBody rigidBody{};
        if (rb.is_object()) {
            if (rb.contains("use_gravity"))      rigidBody.useGravity      = rb["use_gravity"].get<bool>();
            if (rb.contains("density"))          rigidBody.density         = resolveFloat(rb["density"], ctx);
            if (rb.contains("linear_velocity"))  rigidBody.linearVelocity  = resolveVec3(rb["linear_velocity"],  ctx);
            if (rb.contains("angular_velocity")) rigidBody.angularVelocity = resolveVec3(rb["angular_velocity"], ctx);
        }
        if (!reg.has<ecs::RigidBody>(last)) reg.add<ecs::RigidBody>(last, rigidBody);
    }
}

bool SceneSerializer::load(Scene& scene, LightMode& lightMode, const std::string& path, std::optional<uint32_t> forceSeed) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::println(stderr, "[ERROR] Cannot open: {}", path);
        return false;
    }

    json j;
    try { j = json::parse(file, nullptr, true, true); }
    catch (const json::parse_error& e) {
        std::println(stderr, "[ERROR] Parse error: {}", e.what());
        return false;
    }

    scene.clear();

    const int version = j.value("version", -1);
    if (version != SCENE_VERSION) {
        std::println(stderr, "[ERROR] Scene version mismatch in '{}': expected {}, got {}", path, SCENE_VERSION, version);
        return false;
    }

    const uint32_t seed = forceSeed.value_or(
        j.contains("seed") ? j["seed"].get<uint32_t>() : static_cast<uint32_t>(std::random_device{}())
    );
    std::mt19937 rng(seed);

    if (j.contains("light"))
        lightMode = parseLightMode(j["light"].get<std::string>());

    if (j.contains("camera")) {
        const auto& c = j["camera"];
        ResolveCtx camCtx{ rng, {} };

        const bool hasPosition = c.contains("position");
        const bool hasOrbital  = c.contains("radius") || c.contains("azimuth") || c.contains("elevation");
        if (hasPosition && hasOrbital)
            std::println(stderr, "[ERROR] Camera: 'position' and orbital fields ('radius', 'azimuth', 'elevation') are mutually exclusive");

        const glm::vec3 target = c.contains("target") ? resolveVec3(c["target"], camCtx) : glm::vec3(0.0f);

        glm::vec3 pos;
        if (hasOrbital) {
            const float radius    = c.contains("radius")    ? resolveFloat(c["radius"],    camCtx) : 5.0f;
            const float azimuth   = c.contains("azimuth")   ? resolveFloat(c["azimuth"],   camCtx) : 0.0f;
            const float elevation = c.contains("elevation") ? resolveFloat(c["elevation"], camCtx) : 0.0f;
            const float az  = glm::radians(azimuth);
            const float el  = glm::radians(elevation);
            pos = target + radius * glm::vec3(std::cos(el) * std::sin(az), std::sin(el), std::cos(el) * std::cos(az));
        } else {
            pos = hasPosition ? resolveVec3(c["position"], camCtx) : glm::vec3(0.0f, 0.0f, -10.0f);
        }

        const float fov        = c.contains("fov")        ? resolveFloat(c["fov"],        camCtx) : 60.0f;
        const float aperture   = c.contains("aperture")   ? resolveFloat(c["aperture"],   camCtx) : 0.0f;
        const float focusDepth = c.contains("focus_depth") ? resolveFloat(c["focus_depth"], camCtx) : 10.0f;
        const std::string name = c.value("name", "Camera");

        Camera& cam = scene.getCamera();
        cam.setPosition(pos);
        cam.setTarget(target);
        cam.setFov(fov);
        cam.setAperture(aperture);
        cam.setFocusDepth(focusDepth);

        const glm::vec3 dir = glm::length(target - pos) > 1e-6f
            ? glm::normalize(target - pos) : glm::vec3(0.0f, 0.0f, -1.0f);
        const glm::vec3 up  = std::abs(glm::dot(dir, glm::vec3(0,1,0))) < 0.99f
            ? glm::vec3(0,1,0) : glm::vec3(1,0,0);
        const glm::quat rot = glm::quatLookAt(dir, up);
        glm::mat4 mat       = glm::toMat4(rot);
        mat[3]              = glm::vec4(pos, 1.0f);
        scene.pushCamera(name, mat);

        const auto& allEntities = scene.getEntities();
        if (!allEntities.empty()) {
            auto& camStorage = scene.getRegistry().storage<ecs::CameraObject>();
            const ecs::Entity& last = allEntities.back();
            if (camStorage.has(last)) {
                camStorage.get(last).setFov(fov);
                camStorage.get(last).setAperture(aperture);
                camStorage.get(last).setFocusDepth(focusDepth);
            }
        }
    }

    std::unordered_map<std::string, MaterialHandle> matMap;
    if (j.contains("materials")) {
        for (const auto& m : j["materials"]) {
            if (m.contains("repeat")) {
                const int count = m["repeat"].value("count", 1);
                for (int n = 0; n < count; n++) {
                    ResolveCtx ctx{ rng, {{"n", {n, count}}} };
                    Material mat = parseMaterial(m, ctx);
                    matMap[mat.name] = scene.pushMaterial(mat);
                }
            } else {
                ResolveCtx ctx{ rng, {} };
                Material mat = parseMaterial(m, ctx);
                matMap[mat.name] = scene.pushMaterial(mat);
            }
        }
    }

    if (!j.contains("objects")) return true;

    for (const auto& obj : j["objects"]) {
        if (obj.contains("grid")) {
            const auto& g  = obj["grid"];
            const int rows = g.value("rows", 1);
            const int cols = g.value("cols", 1);
            ResolveCtx fixedCtx{ rng, {} };
            const glm::vec3 origin     = g.contains("origin")      ? resolveVec3(g["origin"],      fixedCtx) : glm::vec3(0.0f);
            const glm::vec3 rowSpacing = g.contains("row_spacing") ? resolveVec3(g["row_spacing"], fixedCtx) : glm::vec3(0,0,1);
            const glm::vec3 colSpacing = g.contains("col_spacing") ? resolveVec3(g["col_spacing"], fixedCtx) : glm::vec3(1,0,0);
            for (int r = 0; r < rows; r++) {
                for (int c = 0; c < cols; c++) {
                    const glm::vec3 offset = origin + static_cast<float>(r) * rowSpacing + static_cast<float>(c) * colSpacing;
                    ResolveCtx ctx{ rng, {{"n", {r * cols + c, rows * cols}}, {"row", {r, rows}}, {"col", {c, cols}}} };
                    spawnOne(obj, scene, matMap, ctx, offset);
                }
            }

        } else if (obj.contains("repeat")) {
            const auto& rep   = obj["repeat"];
            const int count   = rep.value("count", 1);
            ResolveCtx fixedCtx{ rng, {} };
            const glm::vec3 step = rep.contains("offset") ? resolveVec3(rep["offset"], fixedCtx) : glm::vec3(0.0f);
            for (int n = 0; n < count; n++) {
                ResolveCtx ctx{ rng, {{"n", {n, count}}} };
                spawnOne(obj, scene, matMap, ctx, static_cast<float>(n) * step);
            }

        } else {
            const std::string name = obj.value("name", "Object");
            if (!obj.contains("type")) {
                std::println(stderr, "[WARNING] '{}': missing 'type' field, skipping", name);
                continue;
            }
            ResolveCtx ctx{ rng, {} };
            spawnOne(obj, scene, matMap, ctx);
        }
    }

    return true;
}

bool SceneSerializer::save(Scene& scene, LightMode lightMode, const std::string& path) {
    json j;
    j["version"] = SCENE_VERSION;
    j["light"]   = toString(lightMode);

    auto& reg        = scene.getRegistry();
    auto& transforms = reg.storage<ecs::Transform>();
    auto& names      = reg.storage<ecs::Name>();
    auto& camObjects = reg.storage<ecs::CameraObject>();

    bool savedCameraEntity = false;
    for (const ecs::Entity& e : scene.getEntities()) {
        if (!camObjects.has(e) || !transforms.has(e)) continue;
        const ecs::CameraObject& c = camObjects.get(e);
        if (c.isPreview) continue;

        const ecs::Transform& t = transforms.get(e);
        const glm::vec3 dir     = glm::normalize(t.rotation * glm::vec3(0.0f, 0.0f, -1.0f));
        const glm::vec3 target  = t.position + dir * glm::max(0.1f, c.focusDepth);

        json cam;
        cam["name"]       = names.has(e) ? trimmed(names.get(e).value) : "Camera";
        cam["position"]   = fromVec3(t.position);
        cam["target"]     = fromVec3(target);
        cam["fov"]        = c.fov;
        cam["aperture"]   = c.aperture;
        cam["focus_depth"] = c.focusDepth;
        j["camera"]       = cam;
        savedCameraEntity = true;
        break;
    }
    if (!savedCameraEntity) {
        const Camera& cam = scene.getCamera();
        json c;
        c["position"]   = fromVec3(cam.getPosition());
        c["target"]     = fromVec3(cam.getTarget());
        c["fov"]        = cam.getFov();
        c["aperture"]   = cam.getAperture();
        c["focus_depth"] = cam.getFocusDepth();
        j["camera"]     = c;
    }

    json matsJson = json::array();
    const auto& mats = scene.getMaterials();
    for (size_t i = 1; i < mats.size(); i++) {
        const Material& m = mats[i];
        json mj;
        mj["name"]   = trimmed(m.name);
        mj["type"]   = toString(m.type);
        mj["albedo"] = fromVec3(m.albedo);
        if (m.roughness        != 0.0f) mj["roughness"]        = m.roughness;
        if (m.metalness        != 0.0f) mj["metalness"]        = m.metalness;
        if (m.ior              != 0.0f) mj["ior"]              = m.ior;
        if (m.transmission     != 0.0f) mj["transmission"]     = m.transmission;
        if (m.emissionStrength != 0.0f) mj["emission_strength"] = m.emissionStrength;
        if (m.density          != 1.0f) mj["density"]          = m.density;
        if (m.anisotropic      != 0.0f) mj["anisotropic"]      = m.anisotropic;
        matsJson.push_back(mj);
    }
    j["materials"] = matsJson;

    auto& materialRefs = reg.storage<ecs::MaterialRef>();
    auto& spheres      = reg.storage<ecs::Sphere>();
    auto& planes       = reg.storage<ecs::Plane>();
    auto& boxes        = reg.storage<ecs::Box>();
    auto& quads        = reg.storage<ecs::Quad>();
    auto& meshRefs     = reg.storage<ecs::MeshRef>();

    auto getMatName = [&](ecs::Entity e) -> std::string {
        if (!materialRefs.has(e)) return "";
        const MaterialHandle h = materialRefs.get(e).handle;
        if (h > 0 && h < (int)mats.size()) return trimmed(mats[h].name);
        return "";
    };

    json objsJson = json::array();
    for (const ecs::Entity& e : scene.getEntities()) {
        if (camObjects.has(e))  continue;
        if (!transforms.has(e)) continue;

        const ecs::Transform& t      = transforms.get(e);
        const std::string entityName = names.has(e) ? trimmed(names.get(e).value) : "Object";
        const std::string matName    = getMatName(e);

        json obj;
        obj["name"] = entityName;

        if (spheres.has(e)) {
            obj["type"]   = "sphere";
            if (!matName.empty()) obj["material"] = matName;
            obj["center"] = fromVec3(t.position);
            obj["radius"] = spheres.get(e).radius;

        } else if (planes.has(e)) {
            obj["type"]   = "plane";
            if (!matName.empty()) obj["material"] = matName;
            obj["point"]  = fromVec3(t.position);
            obj["normal"] = fromVec3(glm::normalize(t.rotation * glm::vec3(0.0f, 1.0f, 0.0f)));

        } else if (boxes.has(e)) {
            obj["type"] = "box";
            if (!matName.empty()) obj["material"] = matName;
            obj["min"]  = fromVec3(t.position - t.scale);
            obj["max"]  = fromVec3(t.position + t.scale);

        } else if (quads.has(e)) {
            const ecs::Quad& q        = quads.get(e);
            const glm::vec3 center    = t.position + 0.5f * (q.u + q.v);
            const glm::vec2 scale     = { glm::length(q.u), glm::length(q.v) };
            const glm::vec3 ref       = std::abs(glm::dot(q.normal, glm::vec3(0,1,0))) < 0.99f
                                        ? glm::vec3(0,1,0) : glm::vec3(1,0,0);
            const glm::vec3 tangent   = glm::normalize(glm::cross(ref, q.normal));
            const glm::vec3 bitangent = glm::normalize(glm::cross(q.normal, tangent));
            const glm::vec3 uHat      = glm::normalize(q.u);
            const float rotation      = std::atan2(glm::dot(uHat, bitangent), glm::dot(uHat, tangent));

            obj["type"]     = "quad";
            if (!matName.empty()) obj["material"] = matName;
            obj["center"]   = fromVec3(center);
            obj["normal"]   = fromVec3(q.normal);
            obj["scale"]    = fromVec2(scale);
            obj["rotation"] = rotation;

        } else if (meshRefs.has(e)) {
            const ecs::MeshRef& ref = meshRefs.get(e);
            const auto& assets = scene.getMeshAssets();
            if (ref.handle < 0 || ref.handle >= (int)assets.size()) continue;
            const std::string meshPath = assets[ref.handle].getPath();
            if (meshPath.empty()) continue;

            obj["type"]     = "mesh";
            if (!matName.empty()) obj["material"] = matName;
            obj["path"]     = meshPath;
            obj["smooth"]   = assets[ref.handle].getSmoothShading();
            obj["position"] = fromVec3(t.position);
            obj["rotation"] = fromVec3(glm::degrees(glm::eulerAngles(t.rotation)));
            obj["scale"]    = fromVec3(t.scale);

        } else {
            continue;
        }

        if (reg.has<ecs::Collider>(e)) {
            const auto& c = reg.get<ecs::Collider>(e);
            json cj;
            if (c.restitution != 0.4f) cj["restitution"] = c.restitution;
            if (c.friction    != 0.4f) cj["friction"]     = c.friction;
            obj["collider"] = cj;
        }

        if (reg.has<ecs::RigidBody>(e)) {
            const auto& rb = reg.get<ecs::RigidBody>(e);
            json rbj;
            if (!rb.useGravity)                              rbj["use_gravity"]       = rb.useGravity;
            if (rb.density != 50.0f)                         rbj["density"]           = rb.density;
            if (rb.linearVelocity  != glm::vec3(0.0f))      rbj["linear_velocity"]   = fromVec3(rb.linearVelocity);
            if (rb.angularVelocity != glm::vec3(0.0f))      rbj["angular_velocity"]  = fromVec3(rb.angularVelocity);
            obj["rigid_body"] = rbj;
        }

        objsJson.push_back(obj);
    }
    j["objects"] = objsJson;

    std::ofstream out(path);
    if (!out.is_open()) {
        std::println(stderr, "[ERROR] Cannot write: {}", path);
        return false;
    }
    out << prettify(j);
    return true;
}
