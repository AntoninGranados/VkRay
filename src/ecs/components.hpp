#pragma once

#include <functional>

#include "./components/material.hpp"
#include "./components/transform.hpp"

#include "./components/objects/sphere.hpp"
#include "./components/objects/plane.hpp"
#include "./components/objects/box.hpp"
#include "./components/objects/mesh.hpp"
#include "./components/objects/camera_object.hpp"

#include "./components/editor/editor_only.hpp"
#include "./components/editor/name.hpp"

#define X(T, Label) { \
        Label, \
        [](ecs::Registry& r, ecs::Entity e) { if (!r.has<T>(e)) r.add<T>(e, T{}); } \
    }

#define ECS_COMPONENTS(X) \
    X(ecs::Name,        "Name"), \
    X(ecs::Transform,   "Transform"), \
    X(ecs::Sphere,      "Sphere"), \
    X(ecs::Plane,       "Plane"), \
    X(ecs::Box,         "Box"), \
    X(ecs::MeshRef,     "Mesh"), \
    X(ecs::MaterialRef, "Material"), \
    X(ecs::CameraObject,"Camera"),

typedef std::function<void(ecs::Registry&, ecs::Entity)> add_function;
static inline const std::vector<std::pair<std::string, add_function> >& componentTypes() {
    static const std::vector<std::pair<std::string, add_function> > types = {
        ECS_COMPONENTS(X)
    };

    return types;
}
