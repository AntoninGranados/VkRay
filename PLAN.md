# Plan

This plan will most likely be subjected to heavy modifications.

## v0.3 Scene & Asset System

- [X] **[0.3.0] Scene file (JSON)**: save/load the full scene to/from a JSON file
  - Covers primitives, materials, camera, and render parameters
  - Replaces the hard-coded reference scene
- [X] **[0.3.1] EXR output**: write renders to OpenEXR instead of PNG only
  - Preserves HDR values for compositing pipelines
  - Natural container for multi-channel AOV data
- [X] **[0.3.2] GBuffer / AOV passes**: expose auxiliary render buffers as outputs
  - Normals, depth, albedo (pixel info buffer already stores these)
  - Enables external denoising workflows (OIDN, OptiX)
- [ ] **[0.3.3] Jobs file (JSON)**: declarative render jobs
  - Scene, output path, SPP, width/height per job
  - Width/height respected in headless; ignored in windowed until v0.5 decouples renderer from editor
  - Enables batch/scriptable renders without recompiling

## v0.4 Camera & Lens

- [ ] **[0.4.0] Motion blur**: time-sampled rays in the pathtracing shader
  - Animation system already in place; needs sub-frame transform interpolation
  - Add support for multiple animation curves (for now they have to be hardcoded); lerp (already), slerp, ease-in/out/in-out, ...
- [ ] **[0.4.1] Arbitrary aperture shape**: replace the disk sample with a user-defined shape
  - Support polygon apertures (square, hex, …) and custom mask textures
  - Controls bokeh blade count and rotation
- [ ] **[0.4.2] Tilted lens (Scheimpflug)**: tilt the focus plane relative to the optical axis
  - Adds `tiltX`/`tiltY` angles to the camera; focus plane intersection replaces the fixed-depth target
  - Enables oblique focus planes and tilt-shift miniature effects

## v0.5 Renderer Architecture

- [ ] **[0.5.0] Renderer / editor separation**: decouple the render pipeline from the editor and display
  - Renderer owns pathtracing and compositing compute passes only; produces `OutputImage`
  - Windowed mode blits `OutputImage` to the swap chain independently of the renderer
  - Editor renders its ImGui overlay on top of the swap chain image, orthogonal to the renderer
  - Headless mode runs the renderer loop directly with no display layer
- [ ] **[0.5.1] Parameter UX**: quality-of-life additions to the parameter system
  - Tooltips on parameter widgets
  - Conditional visibility (hide/grey out params based on other param values)
  - Reset to default value (right-click or Alt+click or other ?)
- [ ] **[0.5.2] Multi-descriptor-set layout**: split the pathtracing pass into 3 descriptor sets by update frequency
  - Requires VkSmol changes: `set` field in `DescriptorBindingDecl`, multi-layout pipeline creation, N-set `vkCmdBindDescriptorSets`
  - Set 0 (per-frame): `PathtracerUBO`, `prevTex`, `PixelInfoBuffer`, `outputImage`
  - Set 1 (scene): all geometry/material SSBOs — updated only on scene edits
  - Set 2 (textures): bindless sampler array — groundwork for v0.8 texture support

## v0.6 Denoising

- [ ] **[0.6.0] Dataset generation**: render and export training pairs (noisy / converged) via the jobs system
  - AOV outputs (normals, albedo, depth) as auxiliary features
  - Scriptable from the jobs file
- [ ] **[0.6.1] Denoiser integration**: run inference on `OutputImage` as a post-process pass
  - Likely OIDN (CPU) or a custom lightweight model trained on the generated dataset

## v0.7 Light Tracing

- [ ] **[0.7.0] Light tracing**: trace paths from emitters and connect to the camera
  - First step toward bidirectional PT
  - Reuses the existing NEE / MIS framework

## v0.8 Textures & Environment

- [ ] **[0.8.0] Texture support**: per-material image textures
  - Albedo, roughness, metallic, normal map
  - Requires a texture atlas or bindless descriptors on the GPU
- [ ] **[0.8.1] HDRI environment map**: lat-long texture replaces the sky gradient
  - Importance sampling over the environment map

## v0.9 Volume Extensions

- [ ] **[0.9.0] Arbitrary density map**: heterogeneous volumes from a 3-D grid
  - OpenVDB or raw `.vdb` / `.nvdb` format
  - Delta tracking replaces the homogeneous sampler
- [ ] **[0.9.1] Subsurface scattering**: random-walk SSS as a BSDF extension
  - Shares infrastructure with the volume scatter code

## Future Ideas

- [ ] **Photon mapping**: store and gather photon hits; spatial hash for lookup; handles caustics and SDS paths
- [ ] **Standard scene import**: USD and glTF interchange formats
- [ ] **Spectral rendering**: replace RGB with wavelength-sampled radiance; enables dispersion, iridescence, fluorescence
- [ ] **Stackless BVH traversal**: [Nvidia Paper](https://research.nvidia.com/sites/default/files/pubs/2010-06_Restart-Trail-for/laine2010hpg_paper.pdf)