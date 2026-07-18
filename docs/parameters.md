# Parameters

> [!NOTE]  
> Parameters are defined in `assets/parameters/parameters.json` as a hierarchical JSON file.  
> Leaf entries require a `"default"` field, from which the parameter type is inferred.  
> Nested objects without `"default"` are display groups and may carry an optional `"label"`.  

## Renderer
| Path | Label | Description | Type | Default | Constraints | Restart |
|------|-------|-------------|------|---------|-------------|---------|
| `render/denoising` | Denoising | - | Boolean | false | - | - |
| `render/debug_view` | Debug View | - | Enumeration | `None` | `None` • `Position W` • `Position` • `Normal W` • `Normal` • `Albedo` • `Roughness` • `Mat Type` • `Bounces` • `Hit Checks` • `Variance` • `Selection Mask` • `Sky Mask` | ✓ |
| `render/max_bounces` | Max Bounces | - | Integer | 8 | 1 ... 20 | - |
| `render/render_samples` | Render Samples | - | Integer | 2048 | 1 ... 4096 | - |
| `render/importance_sampling` | Importance Sampling | - | Boolean | true | - | - |
| `render/clamp` | Clamp Fireflies | Clamps high-luminance samples to reduce fireflies. | Boolean | false | - | ✓ |
| `render/clamp_threshold` | Clamp Threshold | Luminance value above which samples are clamped. | Float | 50 | 0.0 ... 1000.0 | ✓ |
| `render/adaptive_sampling` | Adaptive Sampling | Skips already-converged pixels to focus samples where needed. | Boolean | true | - | - |
| `render/adaptive_warmup` | Adaptive Warmup | Number of samples accumulated before adaptive sampling activates. | Integer | 64 | 0 ... 2048 | - |

### Arbitrary Output Variables
| Path | Label | Description | Type | Default | Constraints | Restart |
|------|-------|-------------|------|---------|-------------|---------|
| `render/aov/position_w` | Position W | World-space hit position. | Boolean | false | - | - |
| `render/aov/position` | Position | Camera-space hit position. | Boolean | false | - | - |
| `render/aov/normal_w` | Normal W | World-space surface normal. | Boolean | false | - | - |
| `render/aov/normal` | Normal | Camera-space surface normal. | Boolean | false | - | - |
| `render/aov/albedo` | Albedo | Unlit surface color. | Boolean | false | - | - |
| `render/aov/roughness` | Roughness | Surface roughness value. | Boolean | false | - | - |
| `render/aov/mat_type` | Mat Type | Encoded material type per pixel. | Boolean | false | - | - |
| `render/aov/sky_mask` | Sky Mask | White for background pixels, black for geometry. | Boolean | false | - | - |

## Scene
| Path | Label | Description | Type | Default | Constraints | Restart |
|------|-------|-------------|------|---------|-------------|---------|
| `scene/light_mode` | Light Mode | - | Enumeration | `Day` | `Day` • `Sunset` • `Night` • `Empty` | ✓ |
