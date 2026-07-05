# Job Format

> [!WARNING]
> This format is under active development. The version number will be bumped on breaking changes with no backward compatibility guarantee.

Job files are JSON files in `res/jobs/`. See existing files there for full examples.

## Top-level

```json
{
    "version": 1,
    "jobs": [ ... ]
}
```

`version` must match the parser's expected version or the file will be rejected.

## Job

```json
{
    "scene":   "res/scenes/my_scene.json",
    "output":  "outputs/render.exr",
    "samples": 1024,
    "width":   1920,
    "height":  1080
}
```

| Field | Values |
|-------|--------|
| `scene` | Path to a scene JSON file. Supports [string tokens](expressions.md#string-tokens). |
| `output` | Output path. Extension sets the format: `.png` or `.exr`. Supports [string tokens](expressions.md#string-tokens). |
| `samples` | Integer SPP, or an array of [checkpoints](#checkpoints). |
| `width`, `height` | Output resolution in pixels. |
| `aovs` | Optional. Array of AOV names to export alongside the main output. See [AOVs](#aovs). |
| `parameters` | Optional. Parameter overrides applied before rendering. See [Parameter overrides](#parameter-overrides). |
| `repeat` | Optional. Runs the job multiple times. See [Repeat](#repeat). |

## AOVs

```json
"aovs": ["normal", "albedo", "depth"]
```

AOV files are written next to the main output with the `_aovs` suffix added (e.g. `render_aovs.exr`).

| Name | Description |
|------|-------------|
| `position_w` | World-space hit position (X, Y, Z channels) |
| `position` | Camera-space hit position (X, Y, Z channels) |
| `normal_w` | World-space normal (X, Y, Z channels) |
| `normal` | Camera-space normal, octahedral (X, Y channels) |
| `albedo` | Surface albedo (R, G, B channels) |
| `roughness` | Surface roughness (V channel) |
| `mat_type` | Material type ID — see `mat_*` constants in `shaders/common.glsl` (V channel) |
| `sky_mask` | Sky hit mask (V channel) |

## Checkpoints

`samples` can be an array of SPP values or checkpoint objects, producing one output file per entry.

```json
"samples": [8, 1024]

"samples": [
    { "spp": 8 },
    { "spp": 1024, "aovs": ["albedo", "normal"] }
]
```

A checkpoint object's `aovs` overrides the job-level `aovs` for that output only. Plain integers inherit the job-level `aovs`.

## Repeat

Runs the job multiple times, each with a different random seed. `{n}` is available in `output` and `scene`.

```json
"repeat": { "count": 20 }
```

## Parameter overrides

```json
"parameters": {
    "pathtracer/sampling/max_bounces": 16,
    "pathtracer/sampling/clip_accumulation": true,
    "pathtracer/sampling/variance_sampling": false,
    ...
}
```

Accepts `bool`, integer, and float values. Keys follow the parameter path hierarchy. All parameters are reset to their defaults before overrides are applied, so settings do not leak between jobs.

See [parameters.md](parameters.md) for the full list of available paths, types, and default values.
