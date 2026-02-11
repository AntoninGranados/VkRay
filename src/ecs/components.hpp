#pragma once


#include <functional>
#include <unordered_map>
#include <list>

#include "./components/material.hpp"
#include "./components/transform.hpp"

#include "./components/objects/sphere.hpp"
#include "./components/objects/plane.hpp"
#include "./components/objects/box.hpp"
#include "./components/objects/mesh.hpp"
#include "./components/objects/camera_object.hpp"

#include "./components/editor/editor_only.hpp"
#include "./components/editor/name.hpp"

#include "./components/animation/transform_anim.hpp"

#define REQ_NONE {}
#define CONFLICT_NONE {}

#define REQ_TRANSFORM ComponentId::Transform
#define CONFLICT_OBJECTS ComponentId::Sphere, ComponentId::Plane, ComponentId::Box, ComponentId::MeshRef, ComponentId::CameraObject

enum class ComponentGroup {
    Editor,
    Objects,
    Movement,
    Other
};

#define ECS_COMPONENTS(X)                                               \
    X(Name,          Editor,   { },               { })                  \
    X(Transform,     Movement, { },               { })                  \
    X(Sphere,        Objects,  { REQ_TRANSFORM }, { CONFLICT_OBJECTS }) \
    X(Plane,         Objects,  { REQ_TRANSFORM }, { CONFLICT_OBJECTS }) \
    X(Box,           Objects,  { REQ_TRANSFORM }, { CONFLICT_OBJECTS }) \
    X(MeshRef,       Objects,  { REQ_TRANSFORM }, { CONFLICT_OBJECTS }) \
    X(CameraObject,  Objects,  { REQ_TRANSFORM }, { CONFLICT_OBJECTS }) \
    X(MaterialRef,   Other,    { },               { })                  \
    X(TransformAnim, Movement, { REQ_TRANSFORM }, { })                  \

#define COMPONENTS_ID(Id, Group, Req, Conflict) Id,
enum class ComponentId {
    ECS_COMPONENTS(COMPONENTS_ID)
};

#define COMPONENTS_LABEL(Id, Group, Req, Conflict) case ComponentId::Id: return #Id;
inline std::string componentLabel(const ComponentId& id) {
    switch (id) {
        ECS_COMPONENTS(COMPONENTS_LABEL)
    }
    return "Unknown";
}

#define COMPONENTS_GROUP(Id, Group, Req, Conflict) case ComponentId::Id: return ComponentGroup::Group;
inline ComponentGroup componentGroup(const ComponentId& id) {
    switch (id) {
        ECS_COMPONENTS(COMPONENTS_GROUP)
    }
    return ComponentGroup::Other;
}

inline std::string componentGroupLabel(ComponentGroup group) {
    switch (group) {
        case ComponentGroup::Editor:   return "Editor";
        case ComponentGroup::Objects:  return "Objects";
        case ComponentGroup::Movement: return "Movement";
        case ComponentGroup::Other:    return "Other";
    }
    return "Other";
}

using AddFunction = std::function<void(ecs::Registry&, ecs::Entity)>;
using HasFunction = std::function<bool(ecs::Registry&, ecs::Entity)>;
struct ComponentFunc {
    AddFunction add;
    HasFunction has;
};

#define COMPONENTS_TYPES(Id, Group, Req, Conflict) {                                                   \
    ComponentId::Id,                                                                                   \
    ComponentFunc {                                                                                    \
        [](ecs::Registry& r, ecs::Entity e) { if (!r.has<ecs::Id>(e)) r.add<ecs::Id>(e, ecs::Id{}); }, \
        [](ecs::Registry& r, ecs::Entity e) { return r.has<ecs::Id>(e); }                              \
    }                                                                                                  \
},
inline std::unordered_map<ComponentId, ComponentFunc>& componentFuncs() {
    static std::unordered_map<ComponentId, ComponentFunc> types = {
        ECS_COMPONENTS(COMPONENTS_TYPES)
    };
    return types;
}

struct ComponentRestrictions {
    std::list<ComponentId> requirements;
    std::list<ComponentId> conflicts;
};
#define COMPONENTS_RESTRICTIONS(Id, Group, Req, Conflict) { \
    ComponentId::Id,                                 \
    ComponentRestrictions { Req, Conflict }          \
},
inline std::unordered_map<ComponentId, ComponentRestrictions>& componentRestrictions() {
    static std::unordered_map<ComponentId, ComponentRestrictions> restrictions = {
        ECS_COMPONENTS(COMPONENTS_RESTRICTIONS)
    };
    return restrictions;
};
