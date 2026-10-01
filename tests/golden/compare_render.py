#!/usr/bin/env python3
import argparse
import json
import subprocess
import sys
import tempfile
import time
from pathlib import Path

import matplotlib
import numpy as np
from PIL import Image

RMSE_THRESHOLD = 0.05
TIME_TOLERANCE = 1.3

REFERENCE_SAMPLES = 131072
# REFERENCE_SAMPLES = 256
FAST_SAMPLES = 1024

SCENE_PATH = "tests/golden/scene.json"

DENOISE_SCENE_OVERRIDE = {
    "Scene": [
        {
            "name": "Compositing",
            "compositing": {
                "passes": [
                    {
                        "name": "ATrous",
                        "path": "assets/compositing/a_trous.glsl",
                        "params": {"cPhi": 0.02},
                    }
                ]
            },
        }
    ]
}

JOB_TEMPLATE = {
    "version": 1,
    "jobs": [
        {
            "scene": SCENE_PATH,
            "render_size": [512, 512],
            "parameters": {
                "renderer/sampling/max_bounces": 15,
                "renderer/sampling/adaptive_sampling": False,
            },
        }
    ],
}


def run_job(vkray_binary, samples, repo_root, clamp, denoise):
    with tempfile.TemporaryDirectory() as tmp:
        tmp_dir = Path(tmp)
        output_path = tmp_dir / "render.png"

        job = json.loads(json.dumps(JOB_TEMPLATE))
        job["jobs"][0]["samples"] = samples
        job["jobs"][0]["output"] = str(output_path)
        job["jobs"][0]["parameters"]["renderer/sampling/clamp"] = clamp
        if clamp:
            job["jobs"][0]["parameters"]["renderer/sampling/clamp_threshold"] = 50.0

        if denoise:
            scene = json.loads((repo_root / SCENE_PATH).read_text())
            scene.update(DENOISE_SCENE_OVERRIDE)
            scene_path = tmp_dir / "scene_denoised.json"
            scene_path.write_text(json.dumps(scene))
            job["jobs"][0]["scene"] = str(scene_path)

        job_path = tmp_dir / "job.json"
        job_path.write_text(json.dumps(job))

        start = time.perf_counter()
        subprocess.run([str(vkray_binary), "--job", str(job_path)], cwd=repo_root, check=True)
        elapsed = time.perf_counter() - start

        render = load_image(output_path)

    return elapsed, render


def load_image(path):
    return np.asarray(Image.open(path).convert("RGB"), dtype=np.float64) / 255.0


def save_image(array, path):
    Image.fromarray(np.clip(array * 255.0, 0, 255).astype(np.uint8)).save(path)


def rmse(a, b):
    return float(np.sqrt(np.mean((a - b) ** 2)))


def error_heatmap(render, reference):
    error = np.abs(render - reference).mean(axis=-1)
    max_error = error.max()
    t = error / max_error if max_error > 0 else error
    return matplotlib.colormaps["viridis"](t)[..., :3]


def history_label(repo_root):
    version = (repo_root / "VERSION").read_text().strip()
    try:
        log = subprocess.run(
            ["git", "log", "--format=%H", "--", "VERSION"],
            cwd=repo_root, capture_output=True, text=True, check=True,
        ).stdout.split()

        origin_commit = None
        for commit in log:
            committed_version = subprocess.run(
                ["git", "show", f"{commit}:VERSION"],
                cwd=repo_root, capture_output=True, text=True, check=True,
            ).stdout.strip()
            if committed_version == version:
                origin_commit = commit
                break

        if origin_commit is None:
            return version

        committed_count = int(subprocess.run(
            ["git", "rev-list", "--count", f"{origin_commit}..HEAD"],
            cwd=repo_root, capture_output=True, text=True, check=True,
        ).stdout.strip())
        count = str(committed_count + 1)
    except subprocess.CalledProcessError:
        count = "1"
    return f"{version}+{count}"


def save_history(golden_dir, repo_root, render, heatmap):
    history_dir = golden_dir / "history" / history_label(repo_root)
    history_dir.mkdir(parents=True, exist_ok=True)
    save_image(render, history_dir / "render.png")
    save_image(heatmap, history_dir / "heatmap.png")
    return history_dir


def update_reference(vkray_binary, repo_root, golden_dir):
    _, render = run_job(vkray_binary, REFERENCE_SAMPLES, repo_root, clamp=False, denoise=False)
    save_image(render, golden_dir / "reference.png")
    print(f"Updated {golden_dir / 'reference.png'}")


def update_baseline_time(vkray_binary, repo_root, golden_dir):
    elapsed, _ = run_job(vkray_binary, FAST_SAMPLES, repo_root, clamp=False, denoise=True)
    (golden_dir / "baseline_time.json").write_text(json.dumps({"seconds": elapsed}, indent=4) + "\n")
    print(f"Updated baseline time ({elapsed:.2f}s)")


def update(vkray_binary, repo_root, golden_dir):
    update_reference(vkray_binary, repo_root, golden_dir)
    update_baseline_time(vkray_binary, repo_root, golden_dir)
    return 0


def check(vkray_binary, repo_root, golden_dir):
    reference_path = golden_dir / "reference.png"
    if not reference_path.exists():
        print(f"No reference image at {reference_path}; run with --update first", file=sys.stderr)
        return 1

    elapsed, render = run_job(vkray_binary, FAST_SAMPLES, repo_root, clamp=False, denoise=True)

    reference = load_image(reference_path)
    if reference.shape != render.shape:
        print(f"Image size mismatch: reference {reference.shape} vs render {render.shape}", file=sys.stderr)
        return 1

    error = rmse(render, reference)
    image_ok = error <= RMSE_THRESHOLD
    print(f"RMSE: {error:.5f} (threshold {RMSE_THRESHOLD})")
    if not image_ok:
        print("FAIL: render differs from the golden reference beyond threshold", file=sys.stderr)

    history_dir = save_history(golden_dir, repo_root, render, error_heatmap(render, reference))
    print(f"Saved render and error heatmap to {history_dir}")

    baseline_path = golden_dir / "baseline_time.json"
    if baseline_path.exists():
        baseline = json.loads(baseline_path.read_text())["seconds"]
        limit = baseline * TIME_TOLERANCE
        time_ok = elapsed <= limit
        print(f"Render time: {elapsed:.2f}s (baseline {baseline:.2f}s, limit {limit:.2f}s)")
        if not time_ok:
            print("FAIL: render time regressed beyond tolerance", file=sys.stderr)
    else:
        time_ok = True
        print(f"No baseline time at {baseline_path}; skipping time check", file=sys.stderr)

    return 0 if (image_ok and time_ok) else 1


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("vkray_binary")
    parser.add_argument("repo_root")
    parser.add_argument("--update", action="store_true", help="Regenerate both the reference image and the timing baseline")
    parser.add_argument("--update-reference", action="store_true", help="Regenerate only the reference image")
    parser.add_argument("--update-baseline-time", action="store_true", help="Regenerate only the timing baseline")
    args = parser.parse_args()

    repo_root = Path(args.repo_root).resolve()
    vkray_binary = Path(args.vkray_binary).resolve()
    golden_dir = repo_root / "tests" / "golden"

    try:
        if args.update:
            return update(vkray_binary, repo_root, golden_dir)
        if args.update_reference:
            update_reference(vkray_binary, repo_root, golden_dir)
            return 0
        if args.update_baseline_time:
            update_baseline_time(vkray_binary, repo_root, golden_dir)
            return 0
        return check(vkray_binary, repo_root, golden_dir)
    except subprocess.CalledProcessError as e:
        print(f"VkRay exited with status {e.returncode}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
