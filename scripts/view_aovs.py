import numpy as np
import matplotlib.pyplot as plt
import OpenEXR
import Imath
from pathlib import Path

class EXRFile:
    def __init__(self, path: Path):
        self._file = OpenEXR.InputFile(str(path))
        header = self._file.header()
        dw = header["dataWindow"]
        self._w = dw.max.x - dw.min.x + 1
        self._h = dw.max.y - dw.min.y + 1
        self._channel_names = set(header["channels"].keys())
        self._pt = Imath.PixelType(Imath.PixelType.FLOAT)

    @property
    def width(self) -> int:
        return self._w

    @property
    def height(self) -> int:
        return self._h

    def has(self, name: str) -> bool:
        return name in self._channel_names

    def _read(self, name: str) -> np.ndarray:
        return np.frombuffer(self._file.channel(name, self._pt), dtype=np.float32).reshape(self._h, self._w)

    def _rgb(self, r: str, g: str, b: str) -> np.ndarray | None:
        if not (self.has(r) and self.has(g) and self.has(b)):
            return None
        return np.stack([self._read(r), self._read(g), self._read(b)], axis=-1)

    def _xy(self, x: str, y: str) -> np.ndarray | None:
        if not (self.has(x) and self.has(y)):
            return None
        return np.stack([self._read(x), self._read(y)], axis=-1)


class AOVFile(EXRFile):
    @property
    def normal(self) -> np.ndarray | None:
        return self._xy("normal.X", "normal.Y")

    @property
    def normal_opaque(self) -> np.ndarray | None:
        return self._xy("normalOpaque.X", "normalOpaque.Y")

    @property
    def albedo(self) -> np.ndarray | None:
        return self._rgb("albedo.R", "albedo.G", "albedo.B")

    @property
    def albedo_opaque(self) -> np.ndarray | None:
        return self._rgb("albedoOpaque.R", "albedoOpaque.G", "albedoOpaque.B")

    @property
    def depth(self) -> np.ndarray | None:
        return self._read("depth.Z") if self.has("depth.Z") else None

    @property
    def depth_opaque(self) -> np.ndarray | None:
        return self._read("depthOpaque.Z") if self.has("depthOpaque.Z") else None

    @property
    def sky_mask(self) -> np.ndarray | None:
        return self._read("skyMask.V") if self.has("skyMask.V") else None

    @property
    def sky_mask_opaque(self) -> np.ndarray | None:
        return self._read("skyMaskOpaque.V") if self.has("skyMaskOpaque.V") else None


class RenderFile(EXRFile):
    @property
    def rgb(self) -> np.ndarray | None:
        return self._rgb("R", "G", "B")


def _show_render(data: np.ndarray) -> np.ndarray:
    return np.clip(data ** (1 / 2.2), 0, 1)


def _show_normal(data: np.ndarray) -> np.ndarray:
    normal = np.stack([data[..., 0], data[..., 1], np.zeros_like(data[..., 0])], axis=-1)
    return np.clip(normal * 0.5 + 0.5, 0, 1)


def _show_depth(data: np.ndarray) -> np.ndarray:
    valid = data > -0.5
    out = np.zeros_like(data)
    if valid.any():
        lo, hi = data[valid].min(), data[valid].max()
        out = np.where(valid, (data - lo) / max(hi - lo, 1e-6), 0.0)
    return out

aov   = AOVFile(Path("scripts/render_aovs.exr"))
noisy = RenderFile(Path("scripts/render.exr")) if Path("scripts/render.exr").exists() else None
clean = RenderFile(Path("scripts/render_clean.exr")) if Path("scripts/render_clean.exr").exists() else None

render_noisy = _show_render(noisy.rgb)
render_clean = _show_render(clean.rgb)
albedo = _show_render(aov.albedo)
albedo_opaque = _show_render(aov.albedo_opaque)
normal = _show_normal(aov.normal)
normal_opaque = _show_normal(aov.normal_opaque)
depth = _show_depth(aov.depth)
depth_opaque = _show_depth(aov.depth_opaque)
sky_mask = aov.sky_mask
sky_mask_opaque = aov.sky_mask_opaque

rows, cols = 2, 6
fig, axes = plt.subplots(rows, cols, figsize=(cols, rows))

axes[0, 0].imshow(render_noisy, origin="upper")
axes[0, 0].set_title("Noisy", fontsize=5)
axes[1, 0].imshow(render_clean, origin="upper")
axes[1, 0].set_title("Clean", fontsize=5)

axes[0, 1].imshow(np.clip(render_noisy / (albedo + 1e-6), 0, 1), origin="upper")
axes[0, 1].set_title("Noisy Demod", fontsize=5)
axes[1, 1].imshow(np.clip(render_clean / (albedo + 1e-6), 0, 1), origin="upper")
axes[1, 1].set_title("Clean Demod", fontsize=5)

axes[0, 2].imshow(albedo, origin="upper")
axes[0, 2].set_title("Albedo", fontsize=5)
axes[1, 2].imshow(albedo_opaque, origin="upper")
axes[1, 2].set_title("Albedo\n (Opaque)", fontsize=5)

axes[0, 3].imshow(normal, origin="upper")
axes[0, 3].set_title("Normal", fontsize=5)
axes[1, 3].imshow(normal_opaque, origin="upper")
axes[1, 3].set_title("Normal\n (Opaque)", fontsize=5)

axes[0, 4].imshow(depth, origin="upper")
axes[0, 4].set_title("Depth", fontsize=5)
axes[1, 4].imshow(depth_opaque, origin="upper")
axes[1, 4].set_title("Depth\n (Opaque)", fontsize=5)

axes[0, 5].imshow(sky_mask, origin="upper")
axes[0, 5].set_title("Sky Mask", fontsize=5)
axes[1, 5].imshow(sky_mask_opaque, origin="upper")
axes[1, 5].set_title("Sky Mask\n (Opaque)", fontsize=5)

for row in axes:
    for ax in row:
        ax.axis('off')

plt.tight_layout()
# plt.show()
plt.savefig("test.png", dpi=900)
