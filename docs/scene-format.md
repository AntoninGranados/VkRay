# Scene Format

> [!WARNING]
> This format is under active development. The version number will be bumped on breaking changes with no backward compatibility guarantee.

Scenes are JSON files (comments allowed) in `assets/scenes/`. See existing files there for full examples.

## Top-level

```json
{
    "version": 1,
    "seed": 42,
    "Scene": [ ... ],
    "Materials": [ ... ],
    "Assets": [ ... ],
    "Objects": [ ... ]
}
```

| Field | Values |
|-------|--------|
| `version` | Must match the loader's version (currently `1`) or the file is rejected. |
| `seed` | Optional integer seed for [`rand`](expressions.md#value-expressions). Omit for a random seed each load. |
| `Scene` | Scene-wide entities: the `Environment` (sky) and the `Compositing` chain. Both are created with defaults when missing. |
| `Materials` | Material entities, referenced by objects through `material_ref`. |
| `Assets` | Asset entities (meshes), referenced by objects through `mesh_ref`. |
| `Objects` | Everything placed in the world: primitives, mesh instances, cameras. |

All sections are optional.

## Nodes

Every entry of a section is a node that becomes one entity. A node is an object keyed by component id, plus a few reserved keys:

```json
{
    "name": "Ball",
    "sphere": {},
    "material_ref": { "handle": "Red" },
    "transform": { "position": [0, 1, 0], "rotation": [0, 45, 0], "scale": [1, 1, 1] },
    "children": [ ... ]
}
```

| Key | Description |
|-----|-------------|
| `name` | Entity name. Supports [string tokens](expressions.md#string-tokens). |
| `children` | Array of nodes parented under this entity. |
| `repeat`, `grid` | Spawn several instances, see [Repeat & Grid](#repeat--grid). |
| `spherical` | Place the entity around a target, see [Camera](#camera). |
| *any other key* | A component id, with its fields as an object. Unknown components and fields are warned about and skipped. |

The full list of components, their fields, types, defaults and constraints is generated in [components.md](components.md). Components a component needs are added automatically (e.g. `principled` adds `material`, `sphere` adds `transform`), and fields left out keep their default. `rotation` is Euler angles in degrees. Enum fields take the zero-based index of the item.

## References

Fields of type `entity` take the name of another entity, and are resolved after the whole file is loaded, so forward references work:

```json
"Materials": [
    { "name": "Red", "principled": { "albedo": [0.8, 0.1, 0.1], "roughness": 0.3 } }
],
"Assets": [
    { "name": "dragon", "mesh": { "path": "../models/dragon.obj", "smooth": true } }
],
"Objects": [
    { "name": "Dragon", "mesh_ref": { "handle": "dragon" }, "material_ref": { "handle": "Red" } }
]
```

Reference names support [string tokens](expressions.md#string-tokens), so repeated objects can point at repeated materials (`"handle": "Mat_{n}"`).

## Paths

Relative file paths (meshes, shader scripts, aperture images, compositing passes) are resolved against the scene file's directory, not the working directory, and are written back relative to it on save. Paths starting with `builtin:/` name files embedded in the executable (e.g. the aperture presets `builtin:/apertures/ring.pgm`) and are kept as-is.

## Programmable components

`programmable` (material), `programmable_sky` and `programmable_lens` load a shader script from `path`. The parameters the script declares are set in the same object as `path`:

```json
"Scene": [
    { "name": "Environment", "programmable_sky": { "path": "../environment/sunset_sky.glsl" } }
],
"Materials": [
    { "name": "Floor", "programmable": { "path": "../materials/voronoi.glsl", "scale": 4.0 } }
]
```

The compositing chain is an ordered list of passes, each with its own script and parameters:

```json
"Scene": [
    {
        "name": "Compositing",
        "compositing": {
            "passes": [
                { "name": "ATrous", "path": "../compositing/a_trous.glsl", "params": { "cPhi": 0.02 } }
            ]
        }
    }
]
```

## Camera

The camera is an entity with a `camera` component and a `transform`. The first camera in the file becomes the active one.

```json
{
    "name": "Camera",
    "camera": { "focal_length": 50, "f_stop": 2.8, "focal_distance": 10 },
    "transform": { "position": [0, 2, -10], "rotation": [-10, 180, 0] }
}
```

For randomized placement, use `spherical` instead of `transform`. It places the entity at `radius`/`azimuth`/`elevation` (degrees) around `target` and orients it towards the target:

```json
{
    "name": "Camera",
    "camera": {},
    "spherical": {
        "radius": { "rand": { "min": 8, "max": 12 } },
        "azimuth": { "rand": { "min": -45, "max": 45 } },
        "elevation": { "rand": { "min": 10, "max": 30 } },
        "target": [0, 0, 0]
    }
}
```

## Repeat & Grid

Spawn several instances of a node. `{n}` (and `{row}`, `{col}` for grids) are available in string fields and as [`lerp`](expressions.md#value-expressions) axes.

```json
{
    "name": "Sphere_{n}",
    "sphere": {},
    "material_ref": { "handle": "Mat_{n}" },
    "transform": { "position": [{ "lerp": { "from": -4, "to": 4, "axis": "n" } }, 0, 0] },
    "repeat": { "count": 5 }
}
```

```json
{
    "name": "Ball_{row}_{col}",
    "sphere": {},
    "transform": {
        "position": [{ "lerp": { "from": -7, "to": 7, "axis": "col" } }, 0, { "lerp": { "from": -3, "to": 3, "axis": "row" } }]
    },
    "grid": { "rows": 4, "cols": 8 }
}
```

## Animation

A field accepts an `anim` array of keyframes instead of a value. Fields marked animatable in [components.md](components.md) are the ones meant to be animated.

```json
"transform": {
    "position": {
        "anim": [
            { "frame": 0, "value": [0, 0, -5] },
            { "frame": 24, "value": [0, 0, 5], "ease": "ease_in_out" }
        ]
    }
}
```

| `ease` | Description |
|--------|-------------|
| *(omit)* | Linear |
| `"step"` | Instant jump at keyframe |
| `"cubic"` | Cubic Hermite |
| `"ease_in"` | Slow start |
| `"ease_out"` | Slow end |
| `"ease_in_out"` | Slow start and end |

Animated fields are evaluated at frame 0 on load.
