# Changelog

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
