# Scene Format

Scenes are JSON files in `scenes/`. See existing files there for full examples.

## Top-level

```json
{
    "version": 0,
    "seed": 42,
    "light": "day",
    "camera": { ... },
    "materials": [ ... ],
    "objects": [ ... ]
}
```

| Field | Values |
|-------|--------|
| `light` | `"day"` `"sunset"` `"night"` `"empty"` |
| `seed` | Integer. Omit for a random seed each load. |

## Camera

```json
"camera": {
    "name": "Camera",
    "position": [0, 2, -10],
    "target": [0, 0, 0],
    "fov": 60,
    "aperture": 0.05,
    "focusDepth": 10
}
```

## Materials

```json
{
    "name": "Mat",
    "type": "ggx_metal",
    "albedo": [1, 0.5, 0],
    "roughness": 0.2
}
```

All types accept `albedo`. All numeric fields support [dynamic values](#dynamic-values).

| `type` | Description | Extra fields |
|--------|-------------|--------------|
| `principled` | General-purpose | `roughness` $\in [0, 1]$, default $0$<br>`metalness` $\in [0, 1]$, default $0$<br>`transmission` $\in [0, 1]$, default $0$<br>`anisotropic` $\in [0, 1]$, default $0$ |
| `emissive` | Light source | `emissionStrength` $\ge 0$, default $0$ |
| `lambertian` | Diffuse | |
| `ggx_metal` | Metallic | `roughness` $\in [0, 1]$, default $0$ |
| `ggx_glossy` | Glossy dielectric | `ior` $> 1$, default $0$<br>`roughness` $\in [0, 1]$, default $0$ |
| `dielectric` | Glass | `ior` $> 1$, default $0$<br>`roughness` $\in [0, 1]$, default $0$<br>`density` $> 0$, default $1$ |
| `volume` | Participating medium | `density` $> 0$, default $1$<br>`anisotropic` $\in [-1, 1]$, default $0$ |

## Objects

```json
{
    "name": "Ball",
    "type": "sphere",
    "material": "Mat",
    "center": [0, 0, 0],
    "radius": 1
}
```

All objects share `name` (string) and `material` (string).

| `type` | Description | Fields |
|--------|-------------|--------|
| `sphere` | Sphere | `center` vec3, default $[0,0,0]$<br>`radius` float, default $1$ |
| `plane` | Infinite plane | `point` vec3, default $[0,0,0]$<br>`normal` vec3, default $[0,1,0]$ |
| `box` | Axis-aligned box | `min` vec3, default $[-1,-1,-1]$<br>`max` vec3, default $[1,1,1]$ |
| `quad` | One-sided rectangle | `center` vec3, default $[0,0,0]$<br>`normal` vec3, default $[0,1,0]$<br>`scale` vec2, default $[1,1]$<br>`rotation` float (radians), default $0$ |
| `mesh` | OBJ mesh | `path` string (required)<br>`position` vec3, default $[0,0,0]$<br>`rotation` vec3 (degrees), default $[0,0,0]$<br>`scale` vec3, default $[1,1,1]$ |

## Dynamic values

Any numeric field accepts `rand` or `lerp` instead of a literal value.

```json
"radius": { "rand": { "min": 0.5, "max": 1.5 } }

"center": { "rand": { "min": [-5, 0, -5], "max": [5, 0, 5] } }

"roughness": { "lerp": { "from": 0.0, "to": 1.0, "axis": "col" } }
```

`lerp` `axis`: `"col"` `"row"` `"n"` — maps the value across the grid or repeat index.

## Repeat

Spawns multiple instances of an object. `{n}` is substituted in `name` and `material`.

```json
{
    "name": "Sphere_{n}",
    "type": "sphere",
    "material": "Mat",
    "center": { "rand": { "min": [-5, 0, -5], "max": [5, 0, 5] } },
    "radius": 1,
    "repeat": { "count": 20, "offset": [0, 2, 0] }
}
```

`offset` steps the position by that vector each iteration. Materials can also repeat:

```json
"materials": [
    {
        "name": "M_{n}",
        "type": "lambertian",
        "albedo": { "rand": { "min": [0, 0, 0], "max": [1, 1, 1] } },
        "repeat": { "count": 20 }
    }
],
"objects": [
    {
        "name": "S_{n}",
        "type": "sphere",
        "material": "M_{n}",
        "radius": 1,
        "repeat": { "count": 20 }
    }
]
```

## Grid

Spawns a 2D grid of instances. `{n}`, `{row}`, `{col}` are available in `name` and `material`.

```json
{
    "name": "Sphere_{row}_{col}",
    "type": "sphere",
    "material": "Mat",
    "radius": 1,
    "grid": {
        "rows": 4, "cols": 8,
        "origin": [0, 0, 0],
        "row_spacing": [0, 0, 2.5],
        "col_spacing": [2.5, 0, 0]
    }
}
```

## Physics

Optional fields on any object.

```json
{
    "name": "Ball",
    "type": "sphere",
    "material": "Mat",
    "center": [0, 5, 0],
    "radius": 1,
    "collider": { "restitution": 0.6, "friction": 0.4 },
    "rigid_body": { "use_gravity": true, "density": 50, "linear_velocity": [0, 0, 0], "angular_velocity": [0, 0, 0] }
}
```
