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

#define REQ_NONE {}
#define CONFLICT_NONE {}

#define REQ_TRANSFORM { ComponentId::Transform }
#define CONFLICT_OBJECTS { ComponentId::Sphere, ComponentId::Plane, ComponentId::Box, ComponentId::MeshRef, ComponentId::CameraObject }

#define ECS_COMPONENTS(X)                               \
    X(Name,         REQ_NONE, CONFLICT_NONE)            \
    X(Transform,    REQ_NONE, CONFLICT_NONE)            \
    X(Sphere,       REQ_TRANSFORM, CONFLICT_OBJECTS)    \
    X(Plane,        REQ_TRANSFORM, CONFLICT_OBJECTS)    \
    X(Box,          REQ_TRANSFORM, CONFLICT_OBJECTS)    \
    X(MeshRef,      REQ_TRANSFORM, CONFLICT_OBJECTS)    \
    X(CameraObject, REQ_TRANSFORM, CONFLICT_OBJECTS)    \
    X(MaterialRef,  REQ_NONE, CONFLICT_NONE)            \

#define COMPONENTS_ID(Id, Req, Conflict) Id,
enum class ComponentId {
    ECS_COMPONENTS(COMPONENTS_ID)
};

#define COMPONENTS_LABEL(Id, Req, Conflict) case ComponentId::Id: return #Id;
inline std::string componentLabel(const ComponentId& id) {
    switch (id) {
        ECS_COMPONENTS(COMPONENTS_LABEL)
    }
    return "Unknown";
}

using AddFunction = std::function<void(ecs::Registry&, ecs::Entity)>;
using HasFunction = std::function<bool(ecs::Registry&, ecs::Entity)>;
struct ComponentFunc {
    AddFunction add;
    HasFunction has;
};

#define COMPONENTS_TYPES(Id, Req, Conflict) {                                                           \
    ComponentId::Id,                                                                                    \
    ComponentFunc {                                                                                     \
        [](ecs::Registry& r, ecs::Entity e) { if (!r.has<ecs::Id>(e)) r.add<ecs::Id>(e, ecs::Id{}); },  \
        [](ecs::Registry& r, ecs::Entity e) { return r.has<ecs::Id>(e); }                               \
    }                                                                                                   \
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
#define COMPONENTS_RESTRICTIONS(Id, Req, Conflict) {           \
    ComponentId::Id,                                           \
    ComponentRestrictions { Req, Conflict }   \
},
inline std::unordered_map<ComponentId, ComponentRestrictions>& componentRestrictions() {
    static std::unordered_map<ComponentId, ComponentRestrictions> restrictions = {
        ECS_COMPONENTS(COMPONENTS_RESTRICTIONS)
    };
    return restrictions;
};
