# Plan

This plan will most likely be subjected to heavy modifications.

## v0.6 Light Tracing

- [ ] **[0.6.0] Spectral rendering**: replace RGB with wavelength-sampled radiance
  - Sample a hero wavelength per ray; carry a small spectral packet (e.g. 4 wavelengths) through the path
  - Spectral sensitivity curves (CIE XYZ or camera RGB primaries) used at accumulation time to convert to display RGB
  - Enables wavelength-dependent effects: dispersion (glass prisms, caustic rainbows), iridescence (thin-film interference), fluorescence
  - Materials need spectral reflectance curves; start with fitted Cauchy/Sellmeier coefficients for dielectrics
- [ ] **[0.6.1] BDPT**: trace paths from emitters and connect to the camera

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