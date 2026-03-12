#!/usr/bin/env python3
"""
Python Baseline: Process CASIA1 dataset with open-iris and save results.

Produces:
  output_dir/
    templates/{subject}_{eye}_{capture}_code.npy   # iris codes (16,256,2) bool
    templates/{subject}_{eye}_{capture}_mask.npy   # mask codes (16,256,2) bool
    genuine_scores.csv      # Same-subject, same-eye matching scores
    impostor_scores.csv     # Different-subject matching scores
    timing.csv              # Per-image processing time
    summary.json            # Aggregate metrics
"""

import csv
import json
import os
import sys
import time
from itertools import combinations
from pathlib import Path

import cv2
import numpy as np


def discover_images(dataset_dir: str) -> list[dict]:
    """Discover all CASIA1 images and parse their metadata."""
    images = []
    for subject_dir in sorted(Path(dataset_dir).iterdir()):
        if not subject_dir.is_dir():
            continue
        for img_file in sorted(subject_dir.glob("*.jpg")):
            # CASIA1 naming: XXX_Y_Z.jpg (subject_eye_capture)
            parts = img_file.stem.split("_")
            if len(parts) != 3:
                continue
            images.append({
                "path": str(img_file),
                "filename": img_file.stem,
                "subject": parts[0],
                "eye": parts[1],
                "capture": parts[2],
                "identity": f"{parts[0]}_{parts[1]}",  # subject+eye = unique iris
            })
    return images


def process_images(images: list[dict], output_dir: str) -> dict:
    """Process all images through open-iris pipeline."""
    import iris

    templates_dir = os.path.join(output_dir, "templates")
    os.makedirs(templates_dir, exist_ok=True)

    pipeline = iris.IRISPipeline()
    results = {}
    timing_data = []
    failures = []

    total = len(images)
    for i, img_info in enumerate(images):
        filename = img_info["filename"]
        print(f"[{i+1}/{total}] Processing {filename}...", end=" ", flush=True)

        img = cv2.imread(img_info["path"], cv2.IMREAD_GRAYSCALE)
        if img is None:
            print("SKIP (cannot read)")
            failures.append({"filename": filename, "error": "cannot read image"})
            continue

        ir_image = iris.IRImage(
            img_data=img, image_id=filename, eye_side="left"
        )

        t0 = time.perf_counter()
        try:
            output = pipeline(ir_image)
        except Exception as e:
            t1 = time.perf_counter()
            print(f"FAIL ({type(e).__name__}: {e})")
            failures.append({
                "filename": filename,
                "error": f"{type(e).__name__}: {e}",
                "time_ms": (t1 - t0) * 1000,
            })
            continue
        t1 = time.perf_counter()
        elapsed_ms = (t1 - t0) * 1000

        if output["error"] is not None:
            print(f"FAIL (pipeline error: {output['error']})")
            failures.append({
                "filename": filename,
                "error": str(output["error"]),
                "time_ms": elapsed_ms,
            })
            continue

        template = output["iris_template"]

        # Save iris codes and mask codes as .npy
        # Combine the list of arrays into a single array for storage
        iris_codes = np.stack(template.iris_codes)   # (2, 16, 256, 2)
        mask_codes = np.stack(template.mask_codes)   # (2, 16, 256, 2)

        code_path = os.path.join(templates_dir, f"{filename}_code.npy")
        mask_path = os.path.join(templates_dir, f"{filename}_mask.npy")
        np.save(code_path, iris_codes)
        np.save(mask_path, mask_codes)

        results[filename] = {
            "template": template,
            "identity": img_info["identity"],
            "subject": img_info["subject"],
            "eye": img_info["eye"],
            "capture": img_info["capture"],
            "time_ms": elapsed_ms,
        }

        timing_data.append({
            "filename": filename,
            "subject": img_info["subject"],
            "eye": img_info["eye"],
            "capture": img_info["capture"],
            "time_ms": elapsed_ms,
        })

        print(f"OK ({elapsed_ms:.1f}ms)")

    return {
        "results": results,
        "timing": timing_data,
        "failures": failures,
    }


def compute_matching_scores(results: dict, output_dir: str) -> dict:
    """Compute genuine and impostor matching scores."""
    from iris.nodes.matcher.hamming_distance_matcher import HammingDistanceMatcher

    matcher = HammingDistanceMatcher(
        rotation_shift=15,
        normalise=True,
        norm_mean=0.45,
        norm_gradient=0.00005,
    )

    # Group templates by identity (subject + eye)
    identity_groups = {}
    for filename, info in results.items():
        identity = info["identity"]
        if identity not in identity_groups:
            identity_groups[identity] = []
        identity_groups[identity].append((filename, info["template"]))

    # Genuine scores: same identity, different captures
    genuine_scores = []
    identities = sorted(identity_groups.keys())
    print(f"\nComputing genuine scores ({len(identities)} identities)...")

    for identity in identities:
        templates = identity_groups[identity]
        for (name_a, tmpl_a), (name_b, tmpl_b) in combinations(templates, 2):
            score = matcher.run(tmpl_a, tmpl_b)
            genuine_scores.append({
                "image_a": name_a,
                "image_b": name_b,
                "identity": identity,
                "score": float(score),
            })

    # Impostor scores: different identities (sample to keep manageable)
    impostor_scores = []
    print(f"Computing impostor scores...")

    # Take first template from each identity for impostor comparisons
    identity_reps = []
    for identity in identities:
        templates = identity_groups[identity]
        identity_reps.append((identity, templates[0][0], templates[0][1]))

    for i in range(len(identity_reps)):
        for j in range(i + 1, len(identity_reps)):
            id_a, name_a, tmpl_a = identity_reps[i]
            id_b, name_b, tmpl_b = identity_reps[j]
            score = matcher.run(tmpl_a, tmpl_b)
            impostor_scores.append({
                "image_a": name_a,
                "image_b": name_b,
                "identity_a": id_a,
                "identity_b": id_b,
                "score": float(score),
            })

    # Save to CSV
    genuine_path = os.path.join(output_dir, "genuine_scores.csv")
    with open(genuine_path, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=["image_a", "image_b", "identity", "score"])
        writer.writeheader()
        writer.writerows(genuine_scores)

    impostor_path = os.path.join(output_dir, "impostor_scores.csv")
    with open(impostor_path, "w", newline="") as f:
        writer = csv.DictWriter(
            f, fieldnames=["image_a", "image_b", "identity_a", "identity_b", "score"]
        )
        writer.writeheader()
        writer.writerows(impostor_scores)

    return {
        "genuine_scores": genuine_scores,
        "impostor_scores": impostor_scores,
    }


def compute_roc_metrics(genuine_scores: list, impostor_scores: list) -> dict:
    """Compute ROC metrics: FNMR at various FMR thresholds."""
    gen = np.array([s["score"] for s in genuine_scores])
    imp = np.array([s["score"] for s in impostor_scores])

    if len(gen) == 0 or len(imp) == 0:
        return {"error": "insufficient data"}

    # Standard FMR thresholds
    fmr_thresholds = [0.1, 0.01, 0.001, 0.0001, 0.00001]
    metrics = {}

    # Sort impostor scores to find thresholds
    imp_sorted = np.sort(imp)

    for fmr_target in fmr_thresholds:
        # Find threshold where FMR = fmr_target
        idx = int(len(imp_sorted) * fmr_target)
        if idx == 0:
            threshold = imp_sorted[0] - 0.001
        else:
            threshold = imp_sorted[idx - 1]

        # FNMR = fraction of genuine scores ABOVE threshold
        fnmr = float(np.mean(gen > threshold))
        actual_fmr = float(np.mean(imp <= threshold))

        metrics[f"FNMR@FMR={fmr_target}"] = {
            "fnmr": fnmr,
            "actual_fmr": actual_fmr,
            "threshold": float(threshold),
        }

    # EER: find threshold where FMR ≈ FNMR
    thresholds = np.linspace(0, 1, 10000)
    best_eer = 1.0
    best_threshold = 0.5
    for t in thresholds:
        fmr = float(np.mean(imp <= t))
        fnmr = float(np.mean(gen > t))
        eer = abs(fmr - fnmr)
        if eer < best_eer:
            best_eer = eer
            best_threshold = float(t)
            best_fmr = fmr
            best_fnmr = fnmr

    metrics["EER"] = {
        "threshold": best_threshold,
        "fmr": best_fmr,
        "fnmr": best_fnmr,
        "eer": (best_fmr + best_fnmr) / 2,
    }

    return metrics


def generate_summary(
    images: list, results: dict, failures: list, timing: list,
    genuine_scores: list, impostor_scores: list, roc_metrics: dict,
    output_dir: str,
) -> dict:
    """Generate and save summary JSON."""
    gen_arr = np.array([s["score"] for s in genuine_scores]) if genuine_scores else np.array([])
    imp_arr = np.array([s["score"] for s in impostor_scores]) if impostor_scores else np.array([])
    time_arr = np.array([t["time_ms"] for t in timing]) if timing else np.array([])

    summary = {
        "pipeline": "Python open-iris v1.11.0",
        "dataset": "CASIA1",
        "total_images": len(images),
        "successful": len(results),
        "failed": len(failures),
        "success_rate": len(results) / len(images) if images else 0,
        "timing": {
            "mean_ms": float(time_arr.mean()) if len(time_arr) > 0 else 0,
            "median_ms": float(np.median(time_arr)) if len(time_arr) > 0 else 0,
            "std_ms": float(time_arr.std()) if len(time_arr) > 0 else 0,
            "min_ms": float(time_arr.min()) if len(time_arr) > 0 else 0,
            "max_ms": float(time_arr.max()) if len(time_arr) > 0 else 0,
            "total_s": float(time_arr.sum() / 1000) if len(time_arr) > 0 else 0,
        },
        "genuine_scores": {
            "count": len(genuine_scores),
            "mean": float(gen_arr.mean()) if len(gen_arr) > 0 else 0,
            "std": float(gen_arr.std()) if len(gen_arr) > 0 else 0,
            "min": float(gen_arr.min()) if len(gen_arr) > 0 else 0,
            "max": float(gen_arr.max()) if len(gen_arr) > 0 else 0,
            "median": float(np.median(gen_arr)) if len(gen_arr) > 0 else 0,
        },
        "impostor_scores": {
            "count": len(impostor_scores),
            "mean": float(imp_arr.mean()) if len(imp_arr) > 0 else 0,
            "std": float(imp_arr.std()) if len(imp_arr) > 0 else 0,
            "min": float(imp_arr.min()) if len(imp_arr) > 0 else 0,
            "max": float(imp_arr.max()) if len(imp_arr) > 0 else 0,
            "median": float(np.median(imp_arr)) if len(imp_arr) > 0 else 0,
        },
        "roc_metrics": roc_metrics,
        "failures": failures,
    }

    summary_path = os.path.join(output_dir, "summary.json")
    with open(summary_path, "w") as f:
        json.dump(summary, f, indent=2)

    return summary


def print_report(summary: dict):
    """Print a human-readable report."""
    print("\n" + "=" * 70)
    print("PYTHON BASELINE REPORT — CASIA1")
    print("=" * 70)

    print(f"\nPipeline:  {summary['pipeline']}")
    print(f"Dataset:   {summary['dataset']}")
    print(f"Images:    {summary['total_images']} total, "
          f"{summary['successful']} OK, {summary['failed']} failed "
          f"({summary['success_rate']*100:.1f}%)")

    t = summary["timing"]
    print(f"\nTiming:    {t['mean_ms']:.1f} ± {t['std_ms']:.1f} ms/image "
          f"(median {t['median_ms']:.1f}, range [{t['min_ms']:.1f}, {t['max_ms']:.1f}])")
    print(f"           Total: {t['total_s']:.1f}s")

    g = summary["genuine_scores"]
    print(f"\nGenuine:   {g['count']} pairs, "
          f"HD = {g['mean']:.4f} ± {g['std']:.4f} "
          f"(median {g['median']:.4f}, range [{g['min']:.4f}, {g['max']:.4f}])")

    i = summary["impostor_scores"]
    print(f"Impostor:  {i['count']} pairs, "
          f"HD = {i['mean']:.4f} ± {i['std']:.4f} "
          f"(median {i['median']:.4f}, range [{i['min']:.4f}, {i['max']:.4f}])")

    roc = summary["roc_metrics"]
    if "error" not in roc:
        print(f"\nROC Metrics:")
        eer = roc.get("EER", {})
        if eer:
            print(f"  EER:             {eer['eer']*100:.2f}% (threshold={eer['threshold']:.4f})")
        for key in sorted(roc.keys()):
            if key.startswith("FNMR"):
                m = roc[key]
                print(f"  {key}: {m['fnmr']*100:.4f}%")

    if summary["failures"]:
        print(f"\nFailures ({len(summary['failures'])}):")
        for f in summary["failures"][:10]:
            print(f"  {f['filename']}: {f['error']}")
        if len(summary["failures"]) > 10:
            print(f"  ... and {len(summary['failures'])-10} more")

    print("\n" + "=" * 70)


def main():
    dataset_dir = sys.argv[1] if len(sys.argv) > 1 else "/Users/innox/projects/iris2/data/Iris/CASIA1"
    output_dir = sys.argv[2] if len(sys.argv) > 2 else "/Users/innox/projects/iris2/iriscpp/biometric/iris/comparison/python_results"

    os.makedirs(output_dir, exist_ok=True)

    print(f"Dataset: {dataset_dir}")
    print(f"Output:  {output_dir}")

    # 1. Discover images
    images = discover_images(dataset_dir)
    print(f"Found {len(images)} images across {len(set(i['subject'] for i in images))} subjects")

    # 2. Process all images
    print("\n--- Processing Images ---")
    proc = process_images(images, output_dir)

    # 3. Save timing
    timing_path = os.path.join(output_dir, "timing.csv")
    with open(timing_path, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=["filename", "subject", "eye", "capture", "time_ms"])
        writer.writeheader()
        writer.writerows(proc["timing"])

    # 4. Compute matching scores
    print("\n--- Computing Matching Scores ---")
    scores = compute_matching_scores(proc["results"], output_dir)

    # 5. Compute ROC metrics
    roc = compute_roc_metrics(scores["genuine_scores"], scores["impostor_scores"])

    # 6. Generate summary
    summary = generate_summary(
        images, proc["results"], proc["failures"], proc["timing"],
        scores["genuine_scores"], scores["impostor_scores"], roc, output_dir,
    )

    # 7. Print report
    print_report(summary)

    print(f"\nResults saved to: {output_dir}")


if __name__ == "__main__":
    main()
