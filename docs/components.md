# Components

Components are defined in `src/core/ecs/components.hpp`.

## Asset

### Mesh
Mesh geometry asset loaded from file.

| Field | Type | Default | Constraints | Animatable |
|-------|------|---------|-------------|------------|
| `path` | path |  |  | no |
| `smooth` | bool | false |  | no |

### Mesh Simplify
Simplifies the mesh asset to a target ratio.

**Needs:** `mesh`

| Field | Type | Default | Constraints | Animatable |
|-------|------|---------|-------------|------------|
| `ratio` | float | 1 | 0.05 ... 1 | no |

## Camera

### Camera
Perspective camera.

**Needs:** `transform` — **Conflicts:** `sphere` `plane` `box` `quad` `mesh_ref`

| Field | Type | Default | Constraints | Animatable |
|-------|------|---------|-------------|------------|
| `fov` | float | 80 | 1 ... 160 | yes |
| `shutter_speed` | float | 0 | ≥ 0 | no |

### Thin Lens
Depth-of-field via thin lens approximation.

**Needs:** `camera`

| Field | Type | Default | Constraints | Animatable |
|-------|------|---------|-------------|------------|
| `focal_length` | float | 21.45 | 1 ... 300 | yes |
| `focal_distance` | float | 10 | ≥ 0.1 | yes |
| `f_stop` | float | 0 | 0 ... 64 | yes |
| `show_focus_plane` | bool | false |  | no |

### Tilt Shift Lens
Tilted focal plane and lens shift (Scheimpflug principle).

**Needs:** `thin_lens`

| Field | Type | Default | Constraints | Animatable |
|-------|------|---------|-------------|------------|
| `plane_position` | vec3 | [0, 0, 0] |  | yes |
| `plane_rotation` | vec3 | [0, 0, 0] |  | yes |

### Geometric Aperture
Polygon aperture blade shape.

**Needs:** `thin_lens` — **Conflicts:** `image_aperture`

| Field | Type | Default | Constraints | Animatable |
|-------|------|---------|-------------|------------|
| `blades` | int | 6 | 3 ... 12 | no |
| `rotation` | float | 0 | 0 ... 360 | no |

### Image Aperture
Custom image mask as aperture shape.

**Needs:** `thin_lens` — **Conflicts:** `geometric_aperture`

| Field | Type | Default | Constraints | Animatable |
|-------|------|---------|-------------|------------|
| `path` | path |  |  | no |

## Material

### Material
Material marker.

 **Conflicts:** `transform`

### Diffuse
Diffuse BSDF.

**Needs:** `material` — **Conflicts:** `emissive` `metal` `glossy` `dielectric` `volume` `principled` `programmable`

| Field | Type | Default | Constraints | Animatable |
|-------|------|---------|-------------|------------|
| `albedo` | vec3 | [0.8, 0.8, 0.8] | 0 ... 1 | yes |

### Emissive
Emissive light source BSDF.

**Needs:** `material` — **Conflicts:** `diffuse` `metal` `glossy` `dielectric` `volume` `principled` `programmable`

| Field | Type | Default | Constraints | Animatable |
|-------|------|---------|-------------|------------|
| `albedo` | vec3 | [1, 1, 1] | 0 ... 1 | yes |
| `emission_strength` | float | 1 | ≥ 0 | yes |

### Metal
GGX metallic BSDF.

**Needs:** `material` — **Conflicts:** `diffuse` `emissive` `glossy` `dielectric` `volume` `principled` `programmable`

| Field | Type | Default | Constraints | Animatable |
|-------|------|---------|-------------|------------|
| `albedo` | vec3 | [0.8, 0.8, 0.8] | 0 ... 1 | yes |
| `roughness` | float | 0.5 | 0 ... 1 | yes |

### Glossy
GGX glossy dielectric BSDF.

**Needs:** `material` — **Conflicts:** `diffuse` `emissive` `metal` `dielectric` `volume` `principled` `programmable`

| Field | Type | Default | Constraints | Animatable |
|-------|------|---------|-------------|------------|
| `albedo` | vec3 | [0.8, 0.8, 0.8] | 0 ... 1 | yes |
| `roughness` | float | 0.5 | 0 ... 1 | yes |
| `ior` | float | 1.5 | 1 ... 3 | yes |

### Dielectric
Dielectric refractive BSDF.

**Needs:** `material` — **Conflicts:** `diffuse` `emissive` `metal` `glossy` `volume` `principled` `programmable`

| Field | Type | Default | Constraints | Animatable |
|-------|------|---------|-------------|------------|
| `albedo` | vec3 | [1, 1, 1] | 0 ... 1 | yes |
| `roughness` | float | 0 | 0 ... 1 | yes |
| `ior` | float | 1.5 | 1 ... 3 | yes |
| `density` | float | 0 | ≥ 0 | yes |
| `transmission` | float | 1 | 0 ... 1 | yes |
| `anisotropic` | float | 0 | -1 ... 1 | yes |

### Volume
Homogeneous participating media BSDF.

**Needs:** `material` — **Conflicts:** `diffuse` `emissive` `metal` `glossy` `dielectric` `principled` `programmable`

| Field | Type | Default | Constraints | Animatable |
|-------|------|---------|-------------|------------|
| `albedo` | vec3 | [0.8, 0.8, 0.8] | 0 ... 1 | yes |
| `density` | float | 1 | ≥ 0 | yes |
| `anisotropic` | float | 0 | -1 ... 1 | yes |

### Principled
PBR principled BSDF.

**Needs:** `material` — **Conflicts:** `diffuse` `emissive` `metal` `glossy` `dielectric` `volume` `programmable`

| Field | Type | Default | Constraints | Animatable |
|-------|------|---------|-------------|------------|
| `albedo` | vec3 | [0.8, 0.8, 0.8] | 0 ... 1 | yes |
| `roughness` | float | 0.5 | 0 ... 1 | yes |
| `metalness` | float | 0 | 0 ... 1 | yes |
| `ior` | float | 1.5 | 1 ... 3 | yes |
| `transmission` | float | 0 | 0 ... 1 | yes |
| `density` | float | 0 | ≥ 0 | yes |
| `anisotropic` | float | 0 | -1 ... 1 | yes |
| `alpha` | float | 1 | 0 ... 1 | yes |

### Programmable
Programmable custom BSDF.

**Needs:** `material` — **Conflicts:** `diffuse` `emissive` `metal` `glossy` `dielectric` `volume` `principled`

| Field | Type | Default | Constraints | Animatable |
|-------|------|---------|-------------|------------|
| `albedo` | vec3 | [0.8, 0.8, 0.8] | 0 ... 1 | yes |

## Movement

### Transform
World-space transform.

| Field | Type | Default | Constraints | Animatable |
|-------|------|---------|-------------|------------|
| `position` | vec3 | [0, 0, 0] |  | yes |
| `rotation` | vec3 | [0, 0, 0] |  | yes |
| `scale` | vec3 | [1, 1, 1] |  | yes |

## Object

### Sphere
Sphere primitive.

**Needs:** `transform` — **Conflicts:** `plane` `box` `mesh_ref` `camera`

| Field | Type | Default | Constraints | Animatable |
|-------|------|---------|-------------|------------|
| `radius` | float | 1 | ≥ 0 | yes |

### Plane
Infinite plane primitive.

**Needs:** `transform` — **Conflicts:** `sphere` `box` `mesh_ref` `quad` `camera`

### Box
Box primitive.

**Needs:** `transform` — **Conflicts:** `sphere` `plane` `mesh_ref` `quad` `camera`

### Quad
Single face quad primitive.

**Needs:** `transform` — **Conflicts:** `sphere` `plane` `box` `mesh_ref` `camera`

### Mesh Ref
Reference to a mesh asset.

**Needs:** `transform` — **Conflicts:** `sphere` `plane` `box` `quad` `camera`

| Field | Type | Default | Constraints | Animatable |
|-------|------|---------|-------------|------------|
| `handle` | entity |  |  | no |

## Other

### Name
Display name.

| Field | Type | Default | Constraints | Animatable |
|-------|------|---------|-------------|------------|
| `value` | string |  |  | no |

### Material Ref
Material reference.

| Field | Type | Default | Constraints | Animatable |
|-------|------|---------|-------------|------------|
| `handle` | entity |  |  | no |

## Physics

### Collider
Physics collider shape.

| Field | Type | Default | Constraints | Animatable |
|-------|------|---------|-------------|------------|
| `restitution` | float | 0.4 | 0 ... 1 | no |
| `friction` | float | 0.4 | 0 ... 1 | no |

### Rigid Body
Physics rigid body.

| Field | Type | Default | Constraints | Animatable |
|-------|------|---------|-------------|------------|
| `use_gravity` | bool | true |  | no |
| `density` | float | 50 | 0.1 ... 10000 | no |
