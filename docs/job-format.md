# Job Format

> [!WARNING]
> This format is under active development. The version number will be bumped on breaking changes with no backward compatibility guarantee.

Job files describe headless renders, run with `./build/VkRay --job <file>`. They are JSON files (comments allowed) in `assets/jobs/`. See existing files there for full examples.

## Top-level

```json
{
    "version": 1,
    "jobs": [ ... ]
}
```

`version` must match the parser's expected version (currently `1`) or the file is rejected. Jobs run in order.

## Job

```json
{
    "scene": "../scenes/my_scene.json",
    "output": "../../outputs/render.exr",
    "samples": 1024,
    "render_size": [1920, 1080]
}
```

| Field | Values |
|-------|--------|
| `scene` | Path to a scene JSON file, relative to the job file's directory. Supports [string tokens](expressions.md#string-tokens). |
| `output` | Output path, relative to the job file's directory. Extension sets the format: `.png` or `.exr`. Supports [string tokens](expressions.md#string-tokens). |
| `samples` | Integer SPP, or an array of [checkpoints](#checkpoints). |
| `render_size` | Optional. `[width, height]` in pixels. Defaults to the `renderer/output/render_size` parameter. |
| `aovs` | Optional. Array of AOV names to export alongside the main output. See [AOVs](#aovs). |
| `parameters` | Optional. Parameter overrides applied before rendering. See [Parameter overrides](#parameter-overrides). |
| `repeat` | Optional. Runs the job multiple times. See [Repeat](#repeat). |

## AOVs

```json
"aovs": ["normal", "albedo", "position"]
```

AOVs are written to a single EXR next to the main output, with `_aovs` added to its name (e.g. `render_aovs.exr`). Pixels where no surface was hit are zero.

| Name | Description |
|------|-------------|
| `position_w` | World-space hit position (X, Y, Z channels) |
| `position` | Camera-space hit position (X, Y, Z channels) |
| `normal_w` | World-space normal (X, Y, Z channels) |
| `normal` | Camera-space normal, octahedral (X, Y channels) |
| `albedo` | Surface albedo (R, G, B channels) |
| `roughness` | Surface roughness (V channel) |
| `mat_type` | Material type ID, `-1` for no hit: `0` principled, `1` emissive, `2` diffuse, `3` metal, `4` glossy, `5` dielectric, `6` volume, `7` programmable (V channel) |
| `sky_mask` | Fraction of samples that hit the sky (V channel) |

## Checkpoints

`samples` can be an array of SPP values or checkpoint objects. Each entry is rendered as its own job, so use the `{spp}` [token](expressions.md#string-tokens) in `output` to keep the files apart.

```json
"output": "../../outputs/render_{spp:low,high}.exr",
"samples": [8, 1024]
```

```json
"samples": [
    { "spp": 8 },
    { "spp": 1024, "aovs": ["albedo", "normal"] }
]
```

A checkpoint object's `aovs` overrides the job-level `aovs` for that output only. Plain integers inherit the job-level `aovs`.

## Repeat

Runs the job multiple times, each with a different random seed (affecting [`rand`](expressions.md#value-expressions) in the scene). `{n}` is available in `output` and `scene`.

```json
"output": "../../outputs/dataset/render_{n}.png",
"repeat": { "count": 20 }
```

## Parameter overrides

```json
"parameters": {
    "renderer/sampling/max_bounces": 16,
    "renderer/sampling/adaptive_sampling": false,
    "renderer/sampling/clamp": true,
    "renderer/sampling/clamp_threshold": 50.0
}
```

Keys are parameter paths. Values can be booleans, integers, floats, strings (enum parameters, by item name), or arrays of 2 to 4 numbers (vector parameters). The JSON type must match the parameter type, so write float values with a decimal point. All parameters are reset to their defaults before each job, so settings do not leak between jobs.

See [parameters.md](parameters.md) for the full list of available paths, types, and default values.
