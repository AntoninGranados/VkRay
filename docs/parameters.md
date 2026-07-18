# Parameters

## Pathtracer
| Path | Label | Type | Default | Constraints | Restart |
|------|-------|------|---------|-------------|---------|
| `pathtracer/debug_view` | Debug View | Enumeration | `None` | `None` • `Position W` • `Position` • `Normal W` • `Normal` • `Albedo` • `Roughness` • `Mat Type` • `Bounces` • `Hit Checks` • `Variance` • `Selection Mask` • `Sky Mask` | ✓ |
| `pathtracer/denoising` | Denoising | Boolean | false | - | - |

### Arbitrary Output Variables
| Path | Label | Type | Default | Constraints | Restart |
|------|-------|------|---------|-------------|---------|
| `pathtracer/aov/albedo` | Albedo | Boolean | false | - | - |
| `pathtracer/aov/mat_type` | Mat Type | Boolean | false | - | - |
| `pathtracer/aov/normal` | Normal | Boolean | false | - | - |
| `pathtracer/aov/normal_w` | Normal W | Boolean | false | - | - |
| `pathtracer/aov/position` | Position | Boolean | false | - | - |
| `pathtracer/aov/position_w` | Position W | Boolean | false | - | - |
| `pathtracer/aov/roughness` | Roughness | Boolean | false | - | - |
| `pathtracer/aov/sky_mask` | Sky Mask | Boolean | false | - | - |

### Sampling
| Path | Label | Type | Default | Constraints | Restart |
|------|-------|------|---------|-------------|---------|
| `pathtracer/sampling/clip_accumulation` | Clip Accumulation | Boolean | false | - | ✓ |
| `pathtracer/sampling/clip_threshold` | Clip Threshold | Float | 50 | 0.0 ... 1000.0 | ✓ |
| `pathtracer/sampling/importance_sampling` | Importance Sampling | Boolean | true | - | - |
| `pathtracer/sampling/max_bounces` | Max Bounces | Integer | 8 | 1 ... 20 | - |
| `pathtracer/sampling/render_samples` | Render Samples | Integer | 2048 | 1 ... 4096 | - |
| `pathtracer/sampling/variance_sampling` | Variance Sampling | Boolean | true | - | - |
| `pathtracer/sampling/variance_warmup` | Variance Warmup Samples | Integer | 64 | 0 ... 2048 | - |

## Scene
| Path | Label | Type | Default | Constraints | Restart |
|------|-------|------|---------|-------------|---------|
| `scene/light_mode` | Light Mode | Enumeration | `Day` | `Day` • `Sunset` • `Night` • `Empty` | ✓ |
