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
- [ ] **[0.3.3] Jobs file (JSON)**: declarative headless render jobs
  - Scene, output path, SPP, resolution per job
  - Enables batch/scriptable renders without recompiling
- [ ] **[0.3.4] Motion blur**: time-sampled rays in the pathtracing shader
  - Animation system already in place; needs sub-frame transform interpolation

## v0.4 Renderer Architecture

- [ ] **[0.4.0] Renderer / editor separation**: decouple the render pipeline from the editor and display
  - Renderer owns pathtracing and compositing compute passes only; produces `OutputImage`
  - Windowed mode blits `OutputImage` to the swap chain independently of the renderer
  - Editor renders its ImGui overlay on top of the swap chain image, orthogonal to the renderer
  - Headless mode runs the renderer loop directly with no display layer
- [ ] **[0.4.1] Multi-descriptor-set layout**: split the pathtracing pass into 3 descriptor sets by update frequency
  - Requires VkSmol changes: `set` field in `DescriptorBindingDecl`, multi-layout pipeline creation, N-set `vkCmdBindDescriptorSets`
  - Set 0 (per-frame): `PathtracerUBO`, `prevTex`, `PixelInfoBuffer`, `outputImage`
  - Set 1 (scene): all geometry/material SSBOs — updated only on scene edits
  - Set 2 (textures): bindless sampler array — groundwork for v0.7 texture support

## v0.5 Denoising

- [ ] **[0.5.0] Dataset generation**: render and export training pairs (noisy / converged) via the jobs system
  - AOV outputs (normals, albedo, depth) as auxiliary features
  - Scriptable from the jobs file
- [ ] **[0.5.1] Denoiser integration**: run inference on `OutputImage` as a post-process pass
  - Likely OIDN (CPU) or a custom lightweight model trained on the generated dataset

## v0.6 Light Tracing

- [ ] **[0.6.0] Light tracing**: trace paths from emitters and connect to the camera
  - First step toward bidirectional PT
  - Reuses the existing NEE / MIS framework

## v0.7 Textures & Environment

- [ ] **[0.7.0] Texture support**: per-material image textures
  - Albedo, roughness, metallic, normal map
  - Requires a texture atlas or bindless descriptors on the GPU
- [ ] **[0.7.1] HDRI environment map**: lat-long texture replaces the sky gradient
  - Importance sampling over the environment map

## v0.8 Volume Extensions

- [ ] **[0.8.0] Arbitrary density map**: heterogeneous volumes from a 3-D grid
  - OpenVDB or raw `.vdb` / `.nvdb` format
  - Delta tracking replaces the homogeneous sampler
- [ ] **[0.8.1] Subsurface scattering**: random-walk SSS as a BSDF extension
  - Shares infrastructure with the volume scatter code

## Future Ideas

- [ ] **Photon mapping**: store and gather photon hits; spatial hash for lookup; handles caustics and SDS paths
- [ ] **Standard scene import**: USD and glTF interchange formats
- [ ] **Spectral rendering**: replace RGB with wavelength-sampled radiance; enables dispersion, iridescence, fluorescence
