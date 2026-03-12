#!/usr/bin/env python3
"""
Visual Verification Gallery for the iris C++ pipeline.

Loads intermediates from comparison/{cpp,python}_results/templates/ and
generates annotated PNG images for human inspection of every pipeline stage.

Usage:
    python visualize_pipeline.py [--images DIR] [--output DIR]

Defaults:
    --images  ../../data/Iris/CASIA1   (CASIA1 dataset for original images)
    --output  gallery                  (output directory for PNG gallery)
"""

import argparse
import os
import re
import sys
from pathlib import Path

import cv2
import matplotlib
matplotlib.use("Agg")  # headless
import matplotlib.pyplot as plt
import numpy as np


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def load_npy_safe(path):
    """Load a .npy file, return None if it doesn't exist."""
    if Path(path).exists():
        return np.load(path)
    return None


def parse_centers(path):
    """Parse eye centers from a text file (key=value format)."""
    centers = {}
    if not Path(path).exists():
        return None
    with open(path) as f:
        for line in f:
            if "=" in line:
                k, v = line.strip().split("=", 1)
                try:
                    centers[k.strip()] = float(v.strip())
                except ValueError:
                    pass
    return centers


def load_original_image(images_dir, name):
    """Load original CASIA1 image by stem name (e.g., '001_1_1').

    CASIA1 directories use bare integers (1, 2, ..., 108) while
    image filenames use zero-padded subjects (001_1_1.jpg).
    """
    if images_dir is None:
        return None
    parts = name.split("_")
    if len(parts) < 3:
        return None
    subject_padded = parts[0]                    # e.g. "001"
    subject_bare = str(int(subject_padded))       # e.g. "1"

    for subdir in [subject_padded, subject_bare]:
        img_path = Path(images_dir) / subdir / f"{name}.jpg"
        if img_path.exists():
            return cv2.imread(str(img_path), cv2.IMREAD_GRAYSCALE)
    return None


def discover_intermediate_images(templates_dir):
    """Find image names that have intermediate files (e.g., norm_image)."""
    names = set()
    for f in Path(templates_dir).glob("*_norm_image.npy"):
        name = f.stem.replace("_py_norm_image", "").replace("_norm_image", "")
        names.add(name)
    return sorted(names)


def eer_threshold(genuine, impostor):
    """Compute EER threshold from genuine/impostor score arrays."""
    thresholds = np.linspace(0, 1, 10000)
    fmr = np.array([np.mean(impostor <= t) for t in thresholds])
    fnmr = np.array([np.mean(genuine > t) for t in thresholds])
    idx = np.argmin(np.abs(fmr - fnmr))
    return thresholds[idx], (fmr[idx] + fnmr[idx]) / 2


# ---------------------------------------------------------------------------
# Plot functions
# ---------------------------------------------------------------------------

def plot_segmentation_overlay(img, seg_preds, output_path, name):
    """Overlay 4-class segmentation on original image."""
    fig, axes = plt.subplots(1, 2, figsize=(14, 5))

    axes[0].imshow(img, cmap="gray")
    axes[0].set_title(f"Original: {name}")
    axes[0].axis("off")

    # seg_preds shape: (H, W, 4) — eyeball, iris, pupil, eyelashes
    class_map = np.argmax(seg_preds, axis=-1)
    colors = np.zeros((*class_map.shape, 4), dtype=np.float32)
    colors[class_map == 0] = [0, 0, 1, 0.3]    # eyeball = blue
    colors[class_map == 1] = [0, 1, 0, 0.4]    # iris = green
    colors[class_map == 2] = [1, 0, 0, 0.4]    # pupil = red
    colors[class_map == 3] = [1, 1, 0, 0.3]    # eyelashes = yellow

    # Resize overlay to match original image if needed
    if img is not None and (colors.shape[0] != img.shape[0] or colors.shape[1] != img.shape[1]):
        colors = cv2.resize(colors, (img.shape[1], img.shape[0]),
                            interpolation=cv2.INTER_NEAREST)

    axes[1].imshow(img, cmap="gray")
    axes[1].imshow(colors)
    axes[1].set_title("Segmentation overlay")
    axes[1].axis("off")

    fig.tight_layout()
    fig.savefig(output_path, dpi=150, bbox_inches="tight")
    plt.close(fig)


def plot_contours_on_image(img, contours, centers, output_path, name, label=""):
    """Overlay geometry contours and eye centers on original image."""
    fig, ax = plt.subplots(1, 1, figsize=(8, 6))
    ax.imshow(img, cmap="gray")

    colors_map = {"iris": "lime", "pupil": "red", "eyeball": "cyan"}
    for cname, pts in contours.items():
        if pts is not None and len(pts) > 0:
            # Close the contour for display
            pts_closed = np.vstack([pts, pts[:1]])
            ax.plot(pts_closed[:, 0], pts_closed[:, 1],
                    color=colors_map.get(cname, "white"), linewidth=1.2,
                    label=cname)

    if centers:
        if "iris_x" in centers:
            ax.plot(centers["iris_x"], centers["iris_y"], "+", color="lime",
                    markersize=12, markeredgewidth=2, label="iris center")
        if "pupil_x" in centers:
            ax.plot(centers["pupil_x"], centers["pupil_y"], "+", color="red",
                    markersize=12, markeredgewidth=2, label="pupil center")

    ax.legend(loc="upper right", fontsize=8)
    ax.set_title(f"Geometry: {name} {label}")
    ax.axis("off")

    fig.tight_layout()
    fig.savefig(output_path, dpi=150, bbox_inches="tight")
    plt.close(fig)


def plot_contours_comparison(img, cpp_contours, py_contours, cpp_centers,
                              py_centers, output_path, name):
    """Side-by-side C++ vs Python contours on same image."""
    fig, axes = plt.subplots(1, 2, figsize=(14, 5))

    for ax, contours, centers, label in [
        (axes[0], cpp_contours, cpp_centers, "C++"),
        (axes[1], py_contours, py_centers, "Python"),
    ]:
        ax.imshow(img, cmap="gray")
        colors_map = {"iris": "lime", "pupil": "red", "eyeball": "cyan"}
        for cname, pts in contours.items():
            if pts is not None and len(pts) > 0:
                pts_closed = np.vstack([pts, pts[:1]])
                ax.plot(pts_closed[:, 0], pts_closed[:, 1],
                        color=colors_map.get(cname, "white"), linewidth=1.2)
        if centers:
            if "iris_x" in centers:
                ax.plot(centers["iris_x"], centers["iris_y"], "+",
                        color="lime", markersize=10, markeredgewidth=2)
            if "pupil_x" in centers:
                ax.plot(centers["pupil_x"], centers["pupil_y"], "+",
                        color="red", markersize=10, markeredgewidth=2)
        ax.set_title(f"{label}: {name}")
        ax.axis("off")

    fig.tight_layout()
    fig.savefig(output_path, dpi=150, bbox_inches="tight")
    plt.close(fig)


def plot_normalized_iris(cpp_norm, py_norm, output_path, name):
    """Side-by-side normalized iris images with diff heatmap."""
    n_rows = 2 if py_norm is None else 3
    fig, axes = plt.subplots(n_rows, 1, figsize=(12, 2.5 * n_rows))
    if n_rows == 1:
        axes = [axes]

    axes[0].imshow(cpp_norm, cmap="gray", aspect="auto")
    axes[0].set_title(f"C++ Normalized Iris: {name}")
    axes[0].axis("off")

    if py_norm is not None:
        axes[1].imshow(py_norm, cmap="gray", aspect="auto")
        axes[1].set_title(f"Python Normalized Iris: {name}")
        axes[1].axis("off")

        # Diff heatmap
        # Ensure same shape
        min_h = min(cpp_norm.shape[0], py_norm.shape[0])
        min_w = min(cpp_norm.shape[1], py_norm.shape[1])
        diff = np.abs(cpp_norm[:min_h, :min_w].astype(np.float64) -
                      py_norm[:min_h, :min_w].astype(np.float64))
        im = axes[2].imshow(diff, cmap="hot", aspect="auto", vmin=0)
        axes[2].set_title(f"Absolute Difference (mean={diff.mean():.4f})")
        axes[2].axis("off")
        fig.colorbar(im, ax=axes[2], shrink=0.6)

    fig.tight_layout()
    fig.savefig(output_path, dpi=150, bbox_inches="tight")
    plt.close(fig)


def plot_iris_code(code, output_path, name, label=""):
    """Binary heatmap of iris code."""
    fig, ax = plt.subplots(1, 1, figsize=(12, 2))
    ax.imshow(code, cmap="gray", aspect="auto", interpolation="nearest")
    ax.set_title(f"Iris Code {label}: {name}  (shape {code.shape})")
    ax.set_xlabel("Column")
    ax.set_ylabel("Row")
    fig.tight_layout()
    fig.savefig(output_path, dpi=150, bbox_inches="tight")
    plt.close(fig)


def plot_code_diff(cpp_code, py_code, output_path, name):
    """XOR diff between C++ and Python iris codes."""
    min_h = min(cpp_code.shape[0], py_code.shape[0])
    min_w = min(cpp_code.shape[1], py_code.shape[1])
    xor_diff = (cpp_code[:min_h, :min_w] != py_code[:min_h, :min_w]).astype(np.uint8)
    pct = 100.0 * xor_diff.sum() / xor_diff.size

    fig, ax = plt.subplots(1, 1, figsize=(12, 2))
    # Show: green = agree, red = differ
    rgb = np.zeros((*xor_diff.shape, 3), dtype=np.float32)
    rgb[xor_diff == 0] = [0.2, 0.8, 0.2]  # green = match
    rgb[xor_diff == 1] = [1.0, 0.0, 0.0]  # red = differ
    ax.imshow(rgb, aspect="auto", interpolation="nearest")
    ax.set_title(f"Code XOR Diff: {name}  ({pct:.1f}% differ, {100-pct:.1f}% agree)")
    ax.axis("off")
    fig.tight_layout()
    fig.savefig(output_path, dpi=150, bbox_inches="tight")
    plt.close(fig)


def plot_score_distributions(genuine, impostor, output_path, label=""):
    """Histogram of genuine vs impostor HD scores with EER line."""
    fig, ax = plt.subplots(1, 1, figsize=(10, 5))

    bins = np.linspace(0, 0.7, 100)
    ax.hist(genuine, bins=bins, alpha=0.6, color="blue", label=f"Genuine (n={len(genuine)})", density=True)
    ax.hist(impostor, bins=bins, alpha=0.6, color="red", label=f"Impostor (n={len(impostor)})", density=True)

    eer_t, eer_val = eer_threshold(genuine, impostor)
    ax.axvline(eer_t, color="green", linestyle="--", linewidth=2,
               label=f"EER threshold={eer_t:.4f} ({eer_val*100:.2f}%)")

    d_prime = abs(np.mean(impostor) - np.mean(genuine)) / np.sqrt(
        (np.std(genuine)**2 + np.std(impostor)**2) / 2)

    ax.set_xlabel("Hamming Distance")
    ax.set_ylabel("Density")
    ax.set_title(f"Score Distributions {label}  |  d'={d_prime:.2f}  EER={eer_val*100:.3f}%")
    ax.legend()
    ax.grid(alpha=0.3)

    fig.tight_layout()
    fig.savefig(output_path, dpi=150, bbox_inches="tight")
    plt.close(fig)


def plot_score_overlay(cpp_gen, cpp_imp, py_gen, py_imp, output_path):
    """Overlay C++ and Python score distributions."""
    fig, ax = plt.subplots(1, 1, figsize=(10, 5))
    bins = np.linspace(0, 0.7, 100)

    ax.hist(cpp_gen, bins=bins, alpha=0.4, color="blue",
            label=f"C++ Genuine (n={len(cpp_gen)})", density=True)
    ax.hist(cpp_imp, bins=bins, alpha=0.4, color="red",
            label=f"C++ Impostor (n={len(cpp_imp)})", density=True)
    ax.hist(py_gen, bins=bins, alpha=0.3, color="cyan", histtype="step",
            linewidth=2, label=f"Python Genuine (n={len(py_gen)})", density=True)
    ax.hist(py_imp, bins=bins, alpha=0.3, color="orange", histtype="step",
            linewidth=2, label=f"Python Impostor (n={len(py_imp)})", density=True)

    ax.set_xlabel("Hamming Distance")
    ax.set_ylabel("Density")
    ax.set_title("C++ vs Python Score Distributions")
    ax.legend(fontsize=8)
    ax.grid(alpha=0.3)

    fig.tight_layout()
    fig.savefig(output_path, dpi=150, bbox_inches="tight")
    plt.close(fig)


def plot_score_correlation(cpp_scores, py_scores, output_path, label=""):
    """Scatter plot of C++ vs Python matching scores."""
    fig, ax = plt.subplots(1, 1, figsize=(7, 7))
    ax.scatter(py_scores, cpp_scores, s=2, alpha=0.3, c="steelblue")
    lims = [0, max(cpp_scores.max(), py_scores.max()) * 1.05]
    ax.plot(lims, lims, "--", color="red", linewidth=1, label="y=x")
    r = np.corrcoef(cpp_scores, py_scores)[0, 1]
    ax.set_xlabel("Python HD")
    ax.set_ylabel("C++ HD")
    ax.set_title(f"Score Correlation {label}  (Pearson r={r:.4f})")
    ax.legend()
    ax.grid(alpha=0.3)
    ax.set_aspect("equal")

    fig.tight_layout()
    fig.savefig(output_path, dpi=150, bbox_inches="tight")
    plt.close(fig)


def plot_bit_agreement_histogram(agreements, output_path):
    """Histogram of per-image bit agreement percentages."""
    fig, ax = plt.subplots(1, 1, figsize=(8, 5))
    ax.hist(agreements, bins=50, color="steelblue", edgecolor="black", alpha=0.7)
    ax.axvline(np.mean(agreements), color="red", linestyle="--",
               label=f"Mean={np.mean(agreements):.2f}%")
    ax.axvline(np.min(agreements), color="orange", linestyle=":",
               label=f"Min={np.min(agreements):.2f}%")
    ax.set_xlabel("Bit Agreement (%)")
    ax.set_ylabel("Count")
    ax.set_title(f"Per-Image Iris Code Bit Agreement (n={len(agreements)})")
    ax.legend()
    ax.grid(alpha=0.3)

    fig.tight_layout()
    fig.savefig(output_path, dpi=150, bbox_inches="tight")
    plt.close(fig)


def plot_full_comparison(img, cpp_norm, py_norm, cpp_code, py_code,
                          cpp_contours, py_contours, output_path, name):
    """Full comparison page: 2 rows (C++, Python) x 3 cols (contours, norm, code)."""
    fig, axes = plt.subplots(2, 3, figsize=(18, 8))

    for row, (contours, norm, code, label) in enumerate([
        (cpp_contours, cpp_norm, cpp_code, "C++"),
        (py_contours, py_norm, py_code, "Python"),
    ]):
        # Contours
        if img is not None:
            axes[row, 0].imshow(img, cmap="gray")
        if contours:
            for cname, pts in contours.items():
                if pts is not None and len(pts) > 0:
                    pts_closed = np.vstack([pts, pts[:1]])
                    c = {"iris": "lime", "pupil": "red", "eyeball": "cyan"}.get(cname, "white")
                    axes[row, 0].plot(pts_closed[:, 0], pts_closed[:, 1],
                                      color=c, linewidth=1)
        axes[row, 0].set_title(f"{label} Contours")
        axes[row, 0].axis("off")

        # Normalized iris
        if norm is not None:
            axes[row, 1].imshow(norm, cmap="gray", aspect="auto")
        axes[row, 1].set_title(f"{label} Normalized Iris")
        axes[row, 1].axis("off")

        # Iris code
        if code is not None:
            axes[row, 2].imshow(code, cmap="gray", aspect="auto",
                                interpolation="nearest")
        axes[row, 2].set_title(f"{label} Iris Code")
        axes[row, 2].axis("off")

    fig.suptitle(f"Cross-Language Comparison: {name}", fontsize=14, y=1.02)
    fig.tight_layout()
    fig.savefig(output_path, dpi=150, bbox_inches="tight")
    plt.close(fig)


# ---------------------------------------------------------------------------
# Template loading (handles C++ and Python format differences)
# ---------------------------------------------------------------------------

def load_cpp_code(templates_dir, name, idx=0):
    """Load C++ iris code: {name}_code_{idx}.npy (uint8, 16x512)."""
    path = Path(templates_dir) / f"{name}_code_{idx}.npy"
    return load_npy_safe(path)


def load_py_code(templates_dir, name):
    """Load Python iris code. Try packed format first, then individual."""
    # Try py_iris_codes.npy (shape: 2, 16, 256, 2 or similar)
    codes = load_npy_safe(Path(templates_dir) / f"{name}_py_iris_codes.npy")
    if codes is not None:
        if codes.ndim == 4 and codes.shape[0] == 2:
            # (2, 16, 256, 2) → take first filter, reshape to (16, 512)
            return codes[0].reshape(16, -1)
        elif codes.ndim == 3:
            return codes[0]
    # Try code.npy
    code = load_npy_safe(Path(templates_dir) / f"{name}_code.npy")
    if code is not None:
        if code.ndim == 4 and code.shape[0] == 2:
            return code[0].reshape(16, -1)
        elif code.ndim == 3:
            return code[0]
        return code
    return None


def load_contours(templates_dir, name, prefix=""):
    """Load geometry contours from NPY files."""
    suffix_map = {
        "iris": f"{prefix}geom_iris",
        "pupil": f"{prefix}geom_pupil",
        "eyeball": f"{prefix}geom_eyeball",
    }
    contours = {}
    for cname, suffix in suffix_map.items():
        path = Path(templates_dir) / f"{name}_{suffix}.npy"
        if not path.exists():
            path = Path(templates_dir) / f"{name}_{suffix}_array.npy"
        arr = load_npy_safe(path)
        contours[cname] = arr
    return contours


def load_scores(results_dir):
    """Load genuine and impostor scores from CSV files."""
    gen_path = Path(results_dir) / "genuine_scores.csv"
    imp_path = Path(results_dir) / "impostor_scores.csv"
    genuine, impostor = [], []

    if gen_path.exists():
        with open(gen_path) as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith("image"):
                    continue
                parts = line.split(",")
                if len(parts) >= 3:
                    try:
                        genuine.append(float(parts[2]))
                    except ValueError:
                        pass

    if imp_path.exists():
        with open(imp_path) as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith("image"):
                    continue
                parts = line.split(",")
                if len(parts) >= 3:
                    try:
                        impostor.append(float(parts[2]))
                    except ValueError:
                        pass

    return np.array(genuine), np.array(impostor)


def compute_bit_agreements(cpp_dir, py_dir):
    """Compute per-image bit agreement between C++ and Python iris codes."""
    agreements = []
    cpp_templates = Path(cpp_dir)
    py_templates = Path(py_dir)

    # Find common template names
    cpp_names = set()
    for f in cpp_templates.glob("*_code_0.npy"):
        cpp_names.add(f.stem.replace("_code_0", ""))

    py_names = set()
    for f in py_templates.glob("*_code.npy"):
        py_names.add(f.stem.replace("_code", ""))
    for f in py_templates.glob("*_py_iris_codes.npy"):
        py_names.add(f.stem.replace("_py_iris_codes", ""))

    common = sorted(cpp_names & py_names)
    for name in common:
        cpp_code = load_cpp_code(str(cpp_templates), name, 0)
        py_code = load_py_code(str(py_templates), name)
        if cpp_code is None or py_code is None:
            continue

        cpp_mask = load_npy_safe(cpp_templates / f"{name}_mask_0.npy")
        py_mask_path = py_templates / f"{name}_mask.npy"
        if not py_mask_path.exists():
            py_mask_path = py_templates / f"{name}_py_mask_codes.npy"
        py_mask = load_npy_safe(py_mask_path)

        # Reshape to common size
        min_h = min(cpp_code.shape[0], py_code.shape[0])
        min_w = min(cpp_code.shape[1], py_code.shape[1])
        c = cpp_code[:min_h, :min_w].astype(np.uint8)
        p = py_code[:min_h, :min_w].astype(np.uint8)

        # Use mask intersection if available
        valid = np.ones((min_h, min_w), dtype=bool)
        if cpp_mask is not None and py_mask is not None:
            cm = cpp_mask[:min_h, :min_w].astype(bool) if cpp_mask.ndim == 2 else True
            if py_mask.ndim == 4:
                pm = py_mask[0].reshape(min_h, -1)[:, :min_w].astype(bool)
            elif py_mask.ndim == 2:
                pm = py_mask[:min_h, :min_w].astype(bool)
            else:
                pm = True
            if isinstance(cm, np.ndarray) and isinstance(pm, np.ndarray):
                valid = cm & pm

        if valid.sum() > 0:
            agree = (c[valid] == p[valid]).sum()
            total = valid.sum()
            agreements.append(100.0 * agree / total)

    return agreements


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(description="Visual Verification Gallery")
    parser.add_argument("--images", type=str, default=None,
                        help="Path to CASIA1 dataset (for original images)")
    parser.add_argument("--output", type=str, default="gallery",
                        help="Output directory for PNG gallery")
    parser.add_argument("--cpp-dir", type=str, default=None,
                        help="Path to C++ templates directory (overrides default)")
    parser.add_argument("--py-dir", type=str, default=None,
                        help="Path to Python templates directory (overrides default)")
    args = parser.parse_args()

    # Resolve paths relative to this script's directory
    script_dir = Path(__file__).resolve().parent

    if args.cpp_dir:
        cpp_templates = Path(args.cpp_dir)
        cpp_results = cpp_templates.parent
    else:
        cpp_results = script_dir / "cpp_results"
        cpp_templates = cpp_results / "templates"

    if args.py_dir:
        py_templates = Path(args.py_dir)
        py_results = py_templates.parent
    else:
        py_results = script_dir / "python_results"
        py_templates = py_results / "templates"

    output_dir = Path(args.output)

    images_dir = args.images
    if images_dir is None:
        # Try default locations
        for candidate in [
            script_dir / "../../data/Iris/CASIA1",
            Path("/data/CASIA1"),
        ]:
            if candidate.exists():
                images_dir = str(candidate)
                break

    # Create output directories
    dirs = [
        "01_segmentation", "02_geometry_contours", "03_normalized_iris",
        "04_iris_code", "05_mask_code", "06_score_distributions",
        "07_cross_language",
    ]
    for d in dirs:
        (output_dir / d).mkdir(parents=True, exist_ok=True)

    # Discover images with intermediates
    intermediate_names = discover_intermediate_images(str(cpp_templates))
    print(f"Found {len(intermediate_names)} images with C++ intermediates: {intermediate_names}")

    # Python intermediates use _py_norm_image.npy naming
    py_intermediate_names = set()
    for f in py_templates.glob("*_py_norm_image.npy"):
        py_intermediate_names.add(f.stem.replace("_py_norm_image", ""))
    py_intermediate_names = sorted(py_intermediate_names)
    common_names = sorted(set(intermediate_names) & set(py_intermediate_names))
    print(f"Found {len(py_intermediate_names)} images with Python intermediates")
    print(f"Common images: {len(common_names)}")

    # ---- Per-image plots ----

    for name in intermediate_names:
        print(f"\nGenerating plots for {name}...")
        img = load_original_image(images_dir, name)

        # 01: Segmentation overlay (Python only)
        seg = load_npy_safe(py_templates / f"{name}_seg_predictions.npy")
        if seg is not None and img is not None:
            plot_segmentation_overlay(
                img, seg,
                output_dir / "01_segmentation" / f"{name}_seg_overlay.png",
                name)
            print(f"  -> 01_segmentation/{name}_seg_overlay.png")

        # 02: Geometry contours
        cpp_contours = load_contours(str(cpp_templates), name)
        cpp_centers = parse_centers(cpp_templates / f"{name}_centers.txt")
        py_contours = load_contours(str(py_templates), name, "")
        py_centers = parse_centers(py_templates / f"{name}_py_centers.txt")

        if img is not None and any(v is not None for v in cpp_contours.values()):
            plot_contours_on_image(
                img, cpp_contours, cpp_centers,
                output_dir / "02_geometry_contours" / f"{name}_cpp_contours.png",
                name, "(C++)")
            print(f"  -> 02_geometry_contours/{name}_cpp_contours.png")

        if img is not None and name in common_names:
            plot_contours_comparison(
                img, cpp_contours, py_contours, cpp_centers, py_centers,
                output_dir / "02_geometry_contours" / f"{name}_comparison.png",
                name)
            print(f"  -> 02_geometry_contours/{name}_comparison.png")

        # 03: Normalized iris
        cpp_norm = load_npy_safe(cpp_templates / f"{name}_norm_image.npy")
        py_norm = load_npy_safe(py_templates / f"{name}_py_norm_image.npy")

        if cpp_norm is not None:
            plot_normalized_iris(
                cpp_norm, py_norm if name in common_names else None,
                output_dir / "03_normalized_iris" / f"{name}_normalized.png",
                name)
            print(f"  -> 03_normalized_iris/{name}_normalized.png")

        # 04: Iris code
        cpp_code0 = load_cpp_code(str(cpp_templates), name, 0)
        cpp_code1 = load_cpp_code(str(cpp_templates), name, 1)
        py_code = load_py_code(str(py_templates), name) if name in common_names else None

        if cpp_code0 is not None:
            plot_iris_code(
                cpp_code0,
                output_dir / "04_iris_code" / f"{name}_cpp_code_0.png",
                name, "C++ filter 0")
            print(f"  -> 04_iris_code/{name}_cpp_code_0.png")

        if cpp_code1 is not None:
            plot_iris_code(
                cpp_code1,
                output_dir / "04_iris_code" / f"{name}_cpp_code_1.png",
                name, "C++ filter 1")

        if py_code is not None:
            plot_iris_code(
                py_code,
                output_dir / "04_iris_code" / f"{name}_py_code.png",
                name, "Python filter 0")

        if cpp_code0 is not None and py_code is not None:
            plot_code_diff(
                cpp_code0, py_code,
                output_dir / "04_iris_code" / f"{name}_code_diff.png",
                name)
            print(f"  -> 04_iris_code/{name}_code_diff.png")

        # 05: Mask code
        cpp_mask0 = load_npy_safe(cpp_templates / f"{name}_mask_0.npy")
        if cpp_mask0 is not None:
            plot_iris_code(
                cpp_mask0,
                output_dir / "05_mask_code" / f"{name}_cpp_mask_0.png",
                name, "C++ mask 0")

    # ---- Aggregate plots ----

    print("\n--- Generating aggregate plots ---")

    # 06: Score distributions
    cpp_gen, cpp_imp = load_scores(str(cpp_results))
    py_gen, py_imp = load_scores(str(py_results))

    if len(cpp_gen) > 0 and len(cpp_imp) > 0:
        plot_score_distributions(
            cpp_gen, cpp_imp,
            output_dir / "06_score_distributions" / "cpp_scores.png",
            "(C++)")
        print("  -> 06_score_distributions/cpp_scores.png")

    if len(py_gen) > 0 and len(py_imp) > 0:
        plot_score_distributions(
            py_gen, py_imp,
            output_dir / "06_score_distributions" / "py_scores.png",
            "(Python)")
        print("  -> 06_score_distributions/py_scores.png")

    if len(cpp_gen) > 0 and len(py_gen) > 0:
        plot_score_overlay(
            cpp_gen, cpp_imp, py_gen, py_imp,
            output_dir / "06_score_distributions" / "overlay.png")
        print("  -> 06_score_distributions/overlay.png")

    # 07: Cross-language analysis
    if len(cpp_gen) > 0 and len(py_gen) > 0:
        # Score correlation (impostor — same pairs)
        min_imp = min(len(cpp_imp), len(py_imp))
        if min_imp > 0:
            plot_score_correlation(
                cpp_imp[:min_imp], py_imp[:min_imp],
                output_dir / "07_cross_language" / "impostor_correlation.png",
                "(Impostor)")
            print("  -> 07_cross_language/impostor_correlation.png")

        min_gen = min(len(cpp_gen), len(py_gen))
        if min_gen > 0:
            plot_score_correlation(
                cpp_gen[:min_gen], py_gen[:min_gen],
                output_dir / "07_cross_language" / "genuine_correlation.png",
                "(Genuine)")
            print("  -> 07_cross_language/genuine_correlation.png")

    # Bit agreement histogram
    print("  Computing bit agreements across all common templates...")
    agreements = compute_bit_agreements(str(cpp_templates), str(py_templates))
    if agreements:
        plot_bit_agreement_histogram(
            agreements,
            output_dir / "07_cross_language" / "bit_agreement_histogram.png")
        print(f"  -> 07_cross_language/bit_agreement_histogram.png  "
              f"(n={len(agreements)}, mean={np.mean(agreements):.2f}%)")

    # Full comparison pages
    for name in common_names:
        img = load_original_image(images_dir, name)
        cpp_norm = load_npy_safe(cpp_templates / f"{name}_norm_image.npy")
        py_norm = load_npy_safe(py_templates / f"{name}_py_norm_image.npy")
        cpp_code0 = load_cpp_code(str(cpp_templates), name, 0)
        py_code = load_py_code(str(py_templates), name)
        cpp_contours = load_contours(str(cpp_templates), name)
        py_contours = load_contours(str(py_templates), name)

        if img is not None:
            plot_full_comparison(
                img, cpp_norm, py_norm, cpp_code0, py_code,
                cpp_contours, py_contours,
                output_dir / "07_cross_language" / f"{name}_full_comparison.png",
                name)
            print(f"  -> 07_cross_language/{name}_full_comparison.png")

    # Summary
    total_files = sum(1 for _ in output_dir.rglob("*.png"))
    print(f"\n{'='*60}")
    print(f"Gallery complete: {total_files} PNG files in {output_dir}/")
    print(f"{'='*60}")


if __name__ == "__main__":
    main()
