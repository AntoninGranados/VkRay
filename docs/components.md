# Components

Components are defined in `src/core/ecs/components/`.

## Asset

### Mesh
Mesh geometry asset loaded from file.

**Kind** · **Permanent**

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
Perspective or orthographic camera with native depth of field.

**Slot:** `shape` · **Needs:** `transform`

| Field | Type | Default | Constraints | Animatable |
|-------|------|---------|-------------|------------|
| `projection` | enum | 0 |  | no |
| `focal_length` | float | 21.45 | 1 ... 300 | yes |
| `sensor_width` | float | 36 | ≥ 1 | yes |
| `sensor_fit` | enum | 2 |  | no |
| `f_stop` | float | 0 | 0 ... 64 | yes |
| `focus_target` | entity |  |  | no |
| `focal_distance` | float | 10 | ≥ 0.1 | yes |
| `show_focus_plane` | bool | false |  | no |
| `shutter_speed` | float | 0 | ≥ 0 | no |

### Tilt Shift Lens
Tilted focal plane and lens shift (Scheimpflug principle).

**Slot:** `lens` · **Needs:** `camera`

| Field | Type | Default | Constraints | Animatable |
|-------|------|---------|-------------|------------|
| `plane_position` | vec3 | [0, 0, 0] |  | yes |
| `plane_rotation` | vec3 | [0, 0, 0] |  | yes |
| `shift_x` | float | 0 |  | yes |
| `shift_y` | float | 0 |  | yes |

### Geometric Aperture
Polygon aperture blade shape.

**Slot:** `aperture` · **Needs:** `camera`

| Field | Type | Default | Constraints | Animatable |
|-------|------|---------|-------------|------------|
| `blades` | int | 6 | 3 ... 12 | no |
| `rotation` | float | 0 | 0 ... 360 | no |

### Image Aperture
Custom image mask as aperture shape.

**Slot:** `aperture` · **Needs:** `camera`

| Field | Type | Default | Constraints | Animatable |
|-------|------|---------|-------------|------------|
| `path` | path |  |  | no |

### Programmable Lens
Programmable custom lens, defined by a GLSL shader-definition file.

**Slot:** `lens` · **Needs:** `camera`

| Field | Type | Default | Constraints | Animatable |
|-------|------|---------|-------------|------------|
| `path` | path |  |  | no |

## Compositing

### Compositing
Compositing chain, an ordered list of programmable passes.

**Kind** · **Permanent**

## Environment

### Environment
Environment marker.

**Kind** · **Permanent**

### Programmable Sky
Programmable custom sky, defined by a GLSL shader-definition file.

**Needs:** `environment`

| Field | Type | Default | Constraints | Animatable |
|-------|------|---------|-------------|------------|
| `path` | path |  |  | no |

## Internal

### Locked
Prevents the entity from being deleted in the editor.

### Camera Navigation
Live interactive navigation state for the active camera.

**Needs:** `camera`

## Material

### Material
Material marker.

**Kind** · **Permanent**

### Material Ref
Material reference.

**Needs:** `transform`

| Field | Type | Default | Constraints | Animatable |
|-------|------|---------|-------------|------------|
| `handle` | entity |  |  | no |

### Diffuse
Diffuse BSDF.

**Slot:** `bsdf` · **Needs:** `material`

| Field | Type | Default | Constraints | Animatable |
|-------|------|---------|-------------|------------|
| `albedo` | vec3 | [0.8, 0.8, 0.8] | 0 ... 1 | yes |

### Emissive
Emissive light source BSDF.

**Slot:** `bsdf` · **Needs:** `material`

| Field | Type | Default | Constraints | Animatable |
|-------|------|---------|-------------|------------|
| `albedo` | vec3 | [1, 1, 1] | 0 ... 1 | yes |
| `emission_strength` | float | 1 | ≥ 0 | yes |

### Metal
GGX metallic BSDF.

**Slot:** `bsdf` · **Needs:** `material`

| Field | Type | Default | Constraints | Animatable |
|-------|------|---------|-------------|------------|
| `albedo` | vec3 | [0.8, 0.8, 0.8] | 0 ... 1 | yes |
| `roughness` | float | 0.5 | 0 ... 1 | yes |

### Glossy
GGX glossy dielectric BSDF.

**Slot:** `bsdf` · **Needs:** `material`

| Field | Type | Default | Constraints | Animatable |
|-------|------|---------|-------------|------------|
| `albedo` | vec3 | [0.8, 0.8, 0.8] | 0 ... 1 | yes |
| `roughness` | float | 0.5 | 0 ... 1 | yes |
| `ior` | float | 1.5 | 1 ... 3 | yes |

### Dielectric
Dielectric refractive BSDF.

**Slot:** `bsdf` · **Needs:** `material`

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

**Slot:** `bsdf` · **Needs:** `material`

| Field | Type | Default | Constraints | Animatable |
|-------|------|---------|-------------|------------|
| `albedo` | vec3 | [0.8, 0.8, 0.8] | 0 ... 1 | yes |
| `density` | float | 1 | ≥ 0 | yes |
| `anisotropic` | float | 0 | -1 ... 1 | yes |

### Principled
PBR principled BSDF.

**Slot:** `bsdf` · **Needs:** `material`

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
Programmable custom BSDF, defined by a GLSL shader-definition file.

**Slot:** `bsdf` · **Needs:** `material`

| Field | Type | Default | Constraints | Animatable |
|-------|------|---------|-------------|------------|
| `path` | path |  |  | no |

## Movement

### Transform
World-space transform.

**Kind** · **Permanent**

| Field | Type | Default | Constraints | Animatable |
|-------|------|---------|-------------|------------|
| `position` | vec3 | [0, 0, 0] |  | yes |
| `rotation` | vec3 | [0, 0, 0] |  | yes |
| `scale` | vec3 | [1, 1, 1] | ≥ 1e-08 | yes |

## Object

### Sphere
Sphere primitive.

**Slot:** `shape` · **Needs:** `transform`

### Plane
Infinite plane primitive.

**Slot:** `shape` · **Needs:** `transform`

### Box
Box primitive.

**Slot:** `shape` · **Needs:** `transform`

### Quad
Single face quad primitive.

**Slot:** `shape` · **Needs:** `transform`

### Mesh Ref
Reference to a mesh asset.

**Slot:** `shape` · **Needs:** `transform`

| Field | Type | Default | Constraints | Animatable |
|-------|------|---------|-------------|------------|
| `handle` | entity |  |  | no |

## Other

### Name
Display name.

**Permanent**

| Field | Type | Default | Constraints | Animatable |
|-------|------|---------|-------------|------------|
| `value` | string |  |  | no |

## Physics

### Collider
Physics collider shape.

**Needs:** `transform`

| Field | Type | Default | Constraints | Animatable |
|-------|------|---------|-------------|------------|
| `restitution` | float | 0.4 | 0 ... 1 | no |
| `friction` | float | 0.4 | 0 ... 1 | no |

### Rigid Body
Physics rigid body.

**Needs:** `transform`

| Field | Type | Default | Constraints | Animatable |
|-------|------|---------|-------------|------------|
| `use_gravity` | bool | true |  | no |
| `density` | float | 50 | 0.1 ... 10000 | no |
