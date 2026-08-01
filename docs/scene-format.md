# Scene Format

> [!WARNING]
> This format is under active development. The version number will be bumped on breaking changes with no backward compatibility guarantee.

Scenes are JSON files in `assets/scenes/`. See existing files there for full examples.

## Top-level

```json
{
    "version": 1,
    "seed": 42,
    "light": "sunset",
    "materials": [ ... ],
    "entities": [ ... ]
}
```

| Field | Values |
|-------|--------|
| `light` | `"day"` `"sunset"` `"night"` `"empty"` |
| `seed` | Integer. Omit for a random seed each load. |

## Entities

Each entity is a JSON object keyed by component id. Unknown keys are warned and skipped.

```json
{
    "sphere": { "radius": 1 },
    "material": "Mat",
    "name": { "value": "Ball" },
    "transform": { "position": [0, 1, 0], "rotation": [0, 45, 0], "scale": [1, 1, 1] }
}
```

The `material` key is a string reference to a material by name. The `mesh` key is an object:

```json
"mesh": { "path": "assets/meshes/foo.obj", "smooth": false }
```

## Camera

The camera is an entity with a `camera` component:

```json
{
    "camera": { "fov": 60, "aperture": 0.05, "focus_depth": 10 },
    "name": { "value": "Camera" },
    "transform": { "position": [0, 2, -10], "rotation": [-10, 0, 0] }
}
```

`rotation` is Euler angles in degrees. The camera looks in `-Z` before rotation.

For randomized placement, use `spherical` instead of `transform`:

```json
{
    "camera": { "fov": 60, "focus_depth": 8 },
    "name": { "value": "Camera" },
    "spherical": {
        "radius": { "rand": { "min": 8, "max": 12 } },
        "azimuth": { "rand": { "min": -45, "max": 45 } },
        "elevation": { "rand": { "min": 10, "max": 30 } },
        "target": [0, 0, 0]
    }
}
```

## Materials

```json
{
    "name": "Mat",
    "type": "ggx_metal",
    "albedo": [1, 0.8, 0.3],
    "roughness": 0.2
}
```

All types accept `albedo`. All numeric fields support [value expressions](expressions.md).

| `type` | Description | Extra fields |
|--------|-------------|--------------|
| `principled` | General-purpose | `roughness` $\in [0, 1]$<br>`metalness` $\in [0, 1]$<br>`transmission` $\in [0, 1]$<br>`anisotropic` $\in [0, 1]$ |
| `emissive` | Light source | `emission_strength` $\ge 0$ |
| `lambertian` | Diffuse | |
| `ggx_metal` | Metallic | `roughness` $\in [0, 1]$ |
| `ggx_glossy` | Glossy dielectric | `ior` $> 1$<br>`roughness` $\in [0, 1]$ |
| `dielectric` | Glass | `ior` $> 1$<br>`roughness` $\in [0, 1]$<br>`density` $> 0$ |
| `volume` | Participating medium | `density` $> 0$<br>`anisotropic` $\in [-1, 1]$ |

## Primitives

Components on an entity that define its shape:

| Component | Fields |
|-----------|--------|
| `sphere` | `radius` float, default $1$ |
| `plane` | — |
| `box` | — |
| `quad` | `scale` vec2, default $[1,1]$ |
| `mesh` | *(see above)* |

All entities with a shape accept a `transform` component (`position`, `rotation`, `scale` — all vec3, defaults $[0,0,0]$, $[0,0,0]$, $[1,1,1]$).

## Physics

Add `collider` and/or `rigid_body` as components:

```json
{
    "sphere": { "radius": 0.5 },
    "material": "Mat",
    "transform": { "position": [0, 5, 0] },
    "collider": { "restitution": 0.6, "friction": 0.4 },
    "rigid_body": {}
}
```

## Repeat & Grid

Spawns multiple instances. `{n}`, `{row}`, `{col}` are available in string fields. See [expressions](expressions.md).

```json
{
    "sphere": { "radius": 0.5 },
    "material": "M_{n}",
    "name": { "value": "Sphere_{n}" },
    "transform": {
        "position": [{ "lerp": { "from": -4, "to": 4, "axis": "n" } }, 0, 0]
    },
    "repeat": { "count": 5 }
}
```

```json
{
    "sphere": {},
    "material": "Mat_{row}_{col}",
    "grid": { "rows": 4, "cols": 8 }
}
```

## Animation

Any numeric field accepts an `anim` array of keyframes instead of a literal value:

```json
"transform": {
    "position": {
        "anim": [
            { "frame": 0,  "value": [0, 0, -5] },
            { "frame": 24, "value": [0, 0,  5], "ease": "ease_in_out" }
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

Material fields also support `anim`. The animated value is evaluated at frame 0 on load.
