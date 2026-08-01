# Components

Components are defined in `src/core/ecs/components.hpp`.

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

**Needs:** `transform` — **Conflicts:** `plane` `box` `mesh` `camera`

| Field | Type | Default | Constraints | Animatable |
|-------|------|---------|-------------|------------|
| `radius` | float | 1 | ≥ 0 | yes |

### Plane
Infinite plane primitive.

**Needs:** `transform` — **Conflicts:** `sphere` `box` `mesh` `quad` `camera`

### Box
Box primitive.

**Needs:** `transform` — **Conflicts:** `sphere` `plane` `mesh` `quad` `camera`

### Quad
Single face quad primitive.

**Needs:** `transform` — **Conflicts:** `sphere` `plane` `box` `mesh` `camera`

### Mesh
Loaded mesh asset.

**Needs:** `transform` — **Conflicts:** `sphere` `plane` `box` `quad` `camera`

| Field | Type | Default | Constraints | Animatable |
|-------|------|---------|-------------|------------|
| `handle` | int | 0 |  | no |

### Camera
Perspective camera.

**Needs:** `transform` — **Conflicts:** `sphere` `plane` `box` `quad` `mesh`

| Field | Type | Default | Constraints | Animatable |
|-------|------|---------|-------------|------------|
| `fov` | float | 80 | 1 ... 160 | yes |
| `aperture` | float | 0 | 0 ... 10 | yes |
| `focus_depth` | float | 10 | ≥ 0 | yes |

## Other

### Name
Display name.

| Field | Type | Default | Constraints | Animatable |
|-------|------|---------|-------------|------------|
| `value` | string |  |  | no |

### Material
Material reference.

| Field | Type | Default | Constraints | Animatable |
|-------|------|---------|-------------|------------|
| `handle` | int | 0 |  | no |

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
