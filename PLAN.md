# Plan

This plan will most likely be subjected to heavy modifications.

## v0.4 Renderer Architecture

- [x] **[0.4.0] Renderer / editor separation**: decouple the render pipeline from the editor and display
  - Renderer owns pathtracing and compositing compute passes only; produces `OutputImage`
  - Windowed mode blits `OutputImage` to the swap chain independently of the renderer
  - Editor renders its ImGui overlay on top of the swap chain image, orthogonal to the renderer
  - Headless mode runs the renderer loop directly with no display layer
- [x] **[0.4.1] Parameter system**: JSON-driven parameter system with a full type-safe API
  - Parameters defined in `assets/parameters/parameters.json` with hierarchical paths
  - Template getter/setter/binder with constrained overloads (scalar, enum, vec)
  - Vec parameter types (ivec2–4, vec2–4) in editor, parser, and job overrides
  - Partial constraint display (omits type-limit bounds); adaptive float precision
- [ ] **[0.4.2] Data-driven ECS components**: define components in JSON rather than hardcoded C++
  - `src/config/components.json` declares each component: id, group, field list (name / type / label / range), requirements, conflicts
  - C++ component structs stay as-is; a thin registration call wires each struct to its schema entry (factory, has, serialise/deserialise, UI draw)
  - `components.hpp` X-macro and hard-coded group/label/restrictions maps replaced by the registry populated from JSON at startup
  - Inspector UI generated from field schemas (same pattern as `ParameterUI`); per-component hand-written UI lambdas removed
  - Scene serialiser accepts both the current shorthand format (`"type": "sphere", ...`) and a new explicit component list format (`"components": ["Transform", "Sphere", ...], "sphere": { ... }`); both expand to the same in-memory entity

## v0.5 Camera & Lens

- [ ] **[0.5.1] Arbitrary aperture shape**: replace the disk sample with a user-defined shape
  - Support polygon apertures (square, hex, ...) and custom mask textures
  - Controls bokeh blade count and rotation
- [ ] **[0.5.2] Tilted lens (Scheimpflug)**: tilt the focus plane relative to the optical axis
  - Adds `tiltX`/`tiltY` angles to the camera; focus plane intersection replaces the fixed-depth target
  - Enables oblique focus planes and tilt-shift miniature effects
- [ ] **[0.5.3] Spectral rendering**: replace RGB with wavelength-sampled radiance
  - Sample a hero wavelength per ray; carry a small spectral packet (e.g. 4 wavelengths) through the path
  - Spectral sensitivity curves (CIE XYZ or camera RGB primaries) used at accumulation time to convert to display RGB
  - Enables wavelength-dependent effects: dispersion (glass prisms, caustic rainbows), iridescence (thin-film interference), fluorescence
  - Materials need spectral reflectance curves; start with fitted Cauchy/Sellmeier coefficients for dielectrics

## v0.6 Light Tracing

- [ ] **[0.6.0] BDPT**: trace paths from emitters and connect to the camera

## v0.X Animation

- [ ] **[0.X.0] Arbitrary interpolation curves**:
  - Add support for multiple animation curves (for now they have to be hardcoded); lerp (already), slerp, ease-in/out/in-out, ...
- [ ] **[0.X.1] Motion blur**: time-sampled rays in the pathtracing shader
- [ ] **[0.X.2] UI Overhaull**

## v0.X Denoising

- [ ] **[0.X.0] Dataset generation**: render and export training pairs (noisy / converged) via the jobs system
  - AOV outputs (normals, albedo, depth) as auxiliary features
  - Scriptable from the jobs file
- [ ] **[0.X.1] Denoiser integration**: run inference on `OutputImage` as a post-process pass
  - Likely OIDN (CPU) or a custom lightweight model trained on the generated dataset

## v0.X Textures & Environment

- [ ] **[0.X.0] Multi-descriptor-set layout**: split the pathtracing pass into 3 descriptor sets by update frequency
  - Requires VkSmol changes: `set` field in `DescriptorBindingDecl`, multi-layout pipeline creation, N-set `vkCmdBindDescriptorSets`
  - Set 0 (per-frame): `PathtracerUBO`, `prevTex`, `PixelInfoBuffer`, `outputImage`
  - Set 1 (scene): all geometry/material SSBOs — updated only on scene edits
  - Set 2 (textures): bindless sampler array — groundwork for v0.8 texture support
- [ ] **[0.X.1] Texture support**: per-material image textures
  - Albedo, roughness, metallic, normal map
  - Requires a texture atlas or bindless descriptors on the GPU
- [ ] **[0.X.2] HDRI environment map**: lat-long texture replaces the sky gradient
  - Importance sampling over the environment map

## v0.X Volume Extensions

- [ ] **[0.X.0] Arbitrary density map**: heterogeneous volumes from a 3-D grid
  - OpenVDB or raw `.vdb` / `.nvdb` format
  - Delta tracking replaces the homogeneous sampler
- [ ] **[0.X.1] Subsurface scattering**: random-walk SSS as a BSDF extension
  - Shares infrastructure with the volume scatter code

## Future Ideas

- [ ] **Photon mapping**: store and gather photon hits; spatial hash for lookup; handles caustics and SDS paths
- [ ] **Standard scene import**: USD and glTF interchange formats
- [ ] **Stackless BVH traversal**: [Nvidia Paper](https://research.nvidia.com/sites/default/files/pubs/2010-06_Restart-Trail-for/laine2010hpg_paper.pdf)
- [ ] **Rastirizer**: when in the editor, render the rasterized scene before using the pathtracer for better interactivity