#!/usr/bin/env python3
import sys
import math
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.image as mpimg
import OpenEXR
import Imath
from pathlib import Path


def read_exr(path: Path) -> dict[str, np.ndarray]:
    f = OpenEXR.InputFile(str(path))
    header = f.header()
    dw = header["dataWindow"]
    h = dw.max.y - dw.min.y + 1
    w = dw.max.x - dw.min.x + 1
    pt = Imath.PixelType(Imath.PixelType.FLOAT)
    return {
        name: np.frombuffer(f.channel(name, pt), dtype=np.float32).reshape(h, w)
        for name in header["channels"]
    }


def group_channels(channels: dict[str, np.ndarray]) -> dict[str, dict[str, np.ndarray]]:
    groups: dict[str, dict[str, np.ndarray]] = {}
    for name, data in channels.items():
        prefix, _, suffix = name.rpartition(".")
        if not prefix:
            prefix, suffix = name, ""
        groups.setdefault(prefix, {})[suffix] = data
    return groups


def _normalize(arr: np.ndarray) -> np.ndarray:
    lo, hi = float(arr.min()), float(arr.max())
    return (arr - lo) / (hi - lo) if hi > lo else np.zeros_like(arr)


def display_group(data: dict[str, np.ndarray]) -> np.ndarray:
    keys = set(data.keys())
    if {"R", "G", "B"} <= keys:
        img = np.stack([data["R"], data["G"], data["B"]], axis=-1)
        return np.clip(img ** (1 / 2.2), 0, 1)
    if {"X", "Y", "Z"} <= keys:
        return np.stack([_normalize(data["X"]), _normalize(data["Y"]), _normalize(data["Z"])], axis=-1)
    if {"X", "Y"} <= keys:
        x, y = data["X"], data["Y"]
        z = np.sqrt(np.maximum(0.0, 1.0 - x**2 - y**2))
        return np.clip(np.stack([x, y, z], axis=-1) * 0.5 + 0.5, 0, 1)
    v = data.get("V", next(iter(data.values())))
    return _normalize(v)


def load_render(path: Path) -> np.ndarray | None:
    if not path.exists():
        return None
    if path.suffix.lower() == ".exr":
        channels = read_exr(path)
        if {"R", "G", "B"} <= set(channels):
            img = np.stack([channels["R"], channels["G"], channels["B"]], axis=-1)
            return np.clip(img ** (1 / 2.2), 0, 1)
        return None
    img = mpimg.imread(str(path))
    if img.dtype == np.uint8:
        img = img.astype(np.float32) / 255.0
    return img[..., :3] if img.ndim == 3 else img


def main():
    if len(sys.argv) < 2:
        print("Usage: view_aovs.py <render.[png|exr]>")
        sys.exit(1)

    src = Path(sys.argv[1])
    aov_path = src.with_name(src.stem + "_aovs.exr")

    if not aov_path.exists():
        print(f"AOV file not found: {aov_path}")
        sys.exit(1)

    channels = read_exr(aov_path)
    groups = group_channels(channels)

    panels: list[tuple[str, np.ndarray]] = []

    render = load_render(src)
    if render is not None:
        panels.append(("render", render))

    for name, data in sorted(groups.items()):
        panels.append((name, display_group(data)))

    n = len(panels)
    cols = math.ceil(math.sqrt(n))
    rows = math.ceil(n / cols)

    fig, axes = plt.subplots(rows, cols, figsize=(cols * 2.2, rows * 2.2))
    axes = np.array(axes).reshape(rows, cols)

    for i, (title, img) in enumerate(panels):
        ax = axes[i // cols, i % cols]
        cmap = "gray" if img.ndim == 2 else None
        ax.imshow(img, origin="upper", cmap=cmap, vmin=0, vmax=1)
        ax.set_title(title, fontsize=7)
        ax.axis("off")

    for i in range(n, rows * cols):
        axes[i // cols, i % cols].axis("off")

    plt.tight_layout()
    out = src.with_name(src.stem + "_view.png")
    plt.savefig(out, dpi=300)
    print(f"Saved: {out}")
    plt.show()


if __name__ == "__main__":
    main()
