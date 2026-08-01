# Expressions

Shared expression syntax used in scene and job JSON files.

## Value expressions

Any numeric field accepts a `rand`, `lerp`, or `anim` object instead of a literal value. Works for scalars, vec2, and vec3.

```json
"radius": { "rand": { "min": 0.5, "max": 1.5 } }

"position": { "rand": { "min": [-5, 0, -5], "max": [5, 0, 5] } }

"roughness": { "lerp": { "from": 0.0, "to": 1.0, "axis": "col" } }

"position": { "anim": [{ "frame": 0, "value": [0, 0, -5] }, { "frame": 24, "value": [0, 0, 5], "ease": "ease_in_out" }] }
```

| Expression | Description |
|------------|-------------|
| `rand` | Uniform random value between `min` and `max`. Re-evaluated each instance. |
| `lerp` | Linearly interpolates `from` → `to` across an index axis. |
| `anim` | Keyframe animation. Array of `{ "frame", "value", "ease" }` objects. See [scene-format](scene-format.md#animation). |

`lerp` `axis`: `"col"` `"row"` `"n"` — maps the value across the grid column, grid row, or repeat index respectively.

## String tokens

String fields such as `name`, `material`, `scene`, and `output` support `{token}` substitutions. Tokens are replaced with zero-padded integers based on the current index and total count.

| Token | Index |
|-------|-------|
| `{n}` | Repeat index |
| `{spp}` | Checkpoint index |
| `{row}` | Grid row index |
| `{col}` | Grid column index |

A label list can be attached to any token with `{token:label0,label1,...}`. If the index is within the label list it is substituted as-is; otherwise the zero-padded integer is used.

```json
"output": "outputs/dataset/out_{n}_{spp:raw,clean}.exr"
```
