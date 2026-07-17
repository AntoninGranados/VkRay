# Changelog

## [0.4.0]

### Added
- Toast notifications; console log panel removed
- Dracula UI theme; cancel button hover color, viewport panel styling

### Changed
- Architecture restructured into three layers: `Core` (engine, scene, parameters), `Editor` (windowed run loop), `Offline` (headless run loop); `Application` reduced to init/teardown only
- `AppContext` removed; subsystems access engine, scene, and parameters through `Core` statics
- `renderFrame()` in `Core` is the shared frame skeleton used by both `Editor` and `Offline`
- Viewport switched to an ImGui panel (was a fullscreen background blit)
- Compositing and Display UBOs split
- `runPreUpdate` / `runPostUpdate` -> `runPreRender` / `runPostRender`
- Project structure: `external/` for libs, `src/shaders/` for shaders, `assets/` for resources
- GLFW and GLM moved to git submodules

### Fixed
- Shadow rays on transmissive and volumetric objects no longer incorrectly apply importance sampling

---

## [0.3.3]

### Added
- Job system for headless batch rendering: declarative JSON files drive multiple renders
    - Scene, output path, SPP, width/height, parameter overrides, and AOV outputs per job
    - Multiple sample checkpoints per job with independent AOV control per checkpoint
- Orbital camera mode for the scene format
- New AOV channels: `roughness` and `mat_type`
- Per-vertex color loaded from `.mtl` files
- Parameter reset to default value
- Progress bar and timer for headless output
- `docs/parameters.md` generated automatically from the parameter definitions
- `README.md`

### Changed
- Parameter system reworked: hierarchical path-based addressing, `bind`/`onSync` callbacks; AOV toggles migrated in
- Scene serializer rewritten; JSON fields now snake_case; scene version field (`version: 1`) added
- AOV buffer restructured; material-type and debug-view constants moved to shared `common.glsl`
- Scenes moved to `res/scenes/`; `res/model/` renamed to `res/models/`
- C++23: `std::format`, `std::println`, `std::unreachable`, `std::to_underlying` throughout

---

## [0.3.2]

### Added
- AOV (Arbitrary Output Variables) export: camera-space 2D normals, albedo, linear depth, sky mask; each with an opaque-hit variant
    - output as a single multi-channel EXR file alongside the beauty render
    - enable/disable checkboxes in the render parameter panel
    - Python viewer (`scripts/view_aovs.py`)

### Changed
- Mesh loading via native file dialog; per-mesh smooth shading toggle
- `PathtracingUBO` split into typed sub-structures (`CameraUBO`, `ScreenUBO`, `FrameUBO`, `RenderUBO`)
- `collectGBuffer` moved out of `traceRay` into the per-sample loop; single traversal now yields both the first hit and the first opaque hit

---

## [0.3.1]

### Added
- EXR output; save dialog opens before the render, format determined by file extension
- JSON scene files now support comments

### Changed
- Headless save unified through `ExportService` (removes separate readback buffer)
- Turn ON support for comments in JSON

---

## [0.3.0]

### Added
- JSON scene format: save/load scenes
    - Procedural scene values: `rand`, `lerp`, `repeat`, `grid`, and `seed` for generative scene descriptions
    - Physics fields (`collider`, `rigid_body`) in the scene format
- Native file dialogs for scene load and save

### Removed
- Hard-coded scene presets; replaced by JSON files in `scenes/`

---

## [0.2.1]

### Fixed
- Volume free-flight sampling: removed double Beer-Lambert in the no-scatter case
- Volume MIS: `prevBsdf` now updated after a scatter event so emitter hits on the next bounce use the HG phase PDF instead of the stale surface BSDF PDF
- NEE shadow rays now treat transmissive surfaces (dielectric and transmissive Principled) as full obstacles; glass no longer incorrectly contributes to direct lighting

---

## [0.2.0]

### Added
- Homogeneous participating media (`mat_Volume`) with Beer-Lambert transmittance, Henyey-Greenstein phase function, and next event estimation at scatter points

### Changed
- Pathtracing shader split into multiple files; medium state decoupled from surface BSDF

---

## [0.1.0]

### Added
- Quad primitive
- Automatic render snapshot on version bump
- `--reference` CLI flag for headless rendering (only render the reference scene)

### Changed
- Camera and input unified under Platform API
- `ReferenceRenderer` removed; headless rendering unified into the main application

---

## [0.0.0] — Initial

### Added
- Vulkan path tracer with BVH (SAH), GGX/Lambertian/Principled/Dielectric BSDFs, importance sampling, Russian Roulette
- Material animation, scene editor UI with gizmos, physics and transform animation systems
- Export service (PNG, video frames)
