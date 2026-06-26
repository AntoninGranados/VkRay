# Changelog

## [0.2.0]

### Added
- Homogeneous participating media (`mat_Volume`)
  - Free-flight sampling with Beer-Lambert transmittance
  - Henyey-Greenstein phase function with importance sampling
  - Next event estimation at scatter points for direct lighting in volumes
  - Shadow ray transmittance through volume boundaries

### Changed
- Pathtracing shader split into `sky.glsl` and `adaptive_sampling.glsl`
- Medium state tracked via persistent `currentMedium` variable, decoupled from surface BSDF; dielectric and volume media handled independently

---

## [0.1.0]

### Added
- Quad primitive: single-sided, defined by normal, origin, and scale
- Cornell box and reference scene walls converted to quads; both scenes are now fully closed
- Automatic render snapshot on version bump, saved to `snapshots/`
- `--reference <path>` CLI flag for headless rendering, following VkSmol's new platform abstraction

### Changed
- Camera and input unified under Platform API
- Reference scene colors neutralized; light remains slightly warm
- `ReferenceRenderer` removed; headless rendering unified into the main application

---

## [0.0.0] — Initial

### Added
- Vulkan path tracer running as a compute shader
- BVH with SAH construction and ordered traversal
- GGX BSDF with true Fresnel, dielectric, Lambertian, Principled BSDF; medium transitions
- Importance sampling; Russian Roulette path termination
- Material animation system
- Scene editor UI with gizmos
- Physics and transform animation systems
- Export service (PNG, video frames)

### Changed
- GPU packing and BVH child storage optimisations
- Fixed false shadow hit in importance sampling
