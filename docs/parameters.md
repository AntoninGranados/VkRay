# Parameters

Parameters are defined in `src/config/parameters.json`. The file is a hierarchical JSON structure where nested objects form display groups, and leaf objects are parameters

## File Format
All objects may carry an optional `"label"` that is used instead of the full path in the UI.

### Groups Format
A nested object without `"default"` is a display group.

```json
"sampling": {
    "label": "Sampling",
    "max_bounces":     { "label": "Max Bounces", "default": 8, "min": 1, "max": 20, "step": 1 },
    "clamp":           { "label": "Clamp Fireflies", "default": false, "restart_accumulation": true },
    "clamp_threshold": {
        "label": "Clamp Threshold",
        "default": 50.0,
        "restart_accumulation": true,
        "condition": { "param": "renderer/sampling/clamp" }
    }
}
```

### Parameters Format
The parameter type is inferred from the value in `"default"`:

| `"default"` value | Type | Example |
|-------------------|------|---------|
| `true` / `false` | Boolean | `"default": false` |
| Integer literal | Integer | `"default": 8` |
| Float literal | Float | `"default": 50.0` |
| Array of 2–4 integers | Vec (integer) | `"default": [1920, 1080]` |
| Array of 2–4 floats | Vec (float) | `"default": [0.0, 0.0, 1.0]` |
| String + `"items"` array | Enumeration | `"default": "None"` |
| String + `"extensions"` array | Path | `"default": "outputs/render.png"` |

All parameters support these fields:

| Field | Description |
|-------|-------------|
| `"label"` | Display name shown in the UI. Required. |
| `"default"` | Default value. Determines the type. Required. |
| `"description"` | Optional tooltip text. |
| `"restart_accumulation"` | If `true`, changing this parameter restarts path tracing. |
| `"condition"` | Disables the parameter unless a condition is met. See [Conditions](#conditions). |

Integer, float, and vec parameters additionally support `"min"`, `"max"`, and `"step"`. For vec parameters, `"min"` and `"max"` are arrays matching the component count; `"step"` is a scalar float.

Enumeration parameters require an `"items"` array listing the valid string values.

Path parameters use an `"extensions"` array of `{ "ext", "name" }` objects to filter the file picker. An empty array makes the picker select a directory instead of a file.

### Conditions
Parameters can be disabled in the UI, this is defined using a `"condition"` object:

```json
"condition": { "param": "renderer/sampling/clamp" }
"condition": { "param": "renderer/sampling/clamp", "when": false }
```

`"param"` is the full path to a boolean parameter and `"when "` is `true` by default.

---

## Renderer
| Path | Label | Description | Type | Default | Constraints | Restart |
|------|-------|-------------|------|---------|-------------|---------|
| `renderer/denoising` | Denoising | - | Boolean | false | - | - |
| `renderer/debug_view` | Debug View | - | Enumeration | `None` | `None` • `Position W` • `Position` • `Normal W` • `Normal` • `Albedo` • `Roughness` • `Mat Type` • `Bounces` • `Hit Checks` • `Variance` • `Selection Mask` • `Sky Mask` | ✓ |

### Sampling
| Path | Label | Description | Type | Default | Constraints | Restart |
|------|-------|-------------|------|---------|-------------|---------|
| `renderer/sampling/max_bounces` | Max Bounces | - | Integer | 8 | 1 ... 20 | - |
| `renderer/sampling/render_samples` | Render Samples | - | Integer | 2048 | 1 ... 4096 | - |
| `renderer/sampling/importance_sampling` | Importance Sampling | - | Boolean | true | - | - |
| `renderer/sampling/clamp` | Clamp Fireflies | Clamps high-luminance samples to reduce fireflies. | Boolean | false | - | ✓ |
| `renderer/sampling/clamp_threshold` | Clamp Threshold | Luminance value above which samples are clamped. | Float | 50 | 0 ... 1000 | ✓ |
| `renderer/sampling/adaptive_sampling` | Adaptive Sampling | Skips already-converged pixels to focus samples where needed. | Boolean | true | - | - |
| `renderer/sampling/adaptive_warmup` | Adaptive Warmup | Number of samples accumulated before adaptive sampling activates. | Integer | 64 | 0 ... 2048 | - |

### Output
| Path | Label | Description | Type | Default | Constraints | Restart |
|------|-------|-------------|------|---------|-------------|---------|
| `renderer/output/render_size` | Render Size | - | Vec2 | (1920, 1080) | (1, 1) ... | - |
| `renderer/output/output_image` | Image | - | Path | `outputs/render.png` | PNG Image (.png), OpenEXR Image (.exr) | - |
| `renderer/output/output_video` | Video | - | Path | `outputs/render.mp4` | MP4 Video (.mp4) | - |
| `renderer/output/frame_cache` | Frame Cache | Directory where animation frames are stored before video conversion. | Path | `outputs/cache` | - | - |

### Arbitrary Output Variables
| Path | Label | Description | Type | Default | Constraints | Restart |
|------|-------|-------------|------|---------|-------------|---------|
| `renderer/aov/position_w` | Position W | World-space hit position. | Boolean | false | - | - |
| `renderer/aov/position` | Position | Camera-space hit position. | Boolean | false | - | - |
| `renderer/aov/normal_w` | Normal W | World-space surface normal. | Boolean | false | - | - |
| `renderer/aov/normal` | Normal | Camera-space surface normal. | Boolean | false | - | - |
| `renderer/aov/albedo` | Albedo | Unlit surface color. | Boolean | false | - | - |
| `renderer/aov/roughness` | Roughness | Surface roughness value. | Boolean | false | - | - |
| `renderer/aov/mat_type` | Mat Type | Encoded material type per pixel. | Boolean | false | - | - |
| `renderer/aov/sky_mask` | Sky Mask | White for background pixels, black for geometry. | Boolean | false | - | - |

## Scene
| Path | Label | Description | Type | Default | Constraints | Restart |
|------|-------|-------------|------|---------|-------------|---------|
| `scene/light_mode` | Light Mode | - | Enumeration | `Day` | `Day` • `Sunset` • `Night` • `Empty` | ✓ |
