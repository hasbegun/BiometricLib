#!/usr/bin/env python3
"""
Cross-Language Comparison: Compare Python and C++ iris pipeline results.

Reads:
  python_results/   (from python_baseline.py)
  cpp_results/      (from iris_comparison C++ tool)

Produces:
  comparison_report.md   — formatted comparison report
"""

import csv
import json
import os
import sys
from pathlib import Path

import numpy as np


def load_csv_scores(path: str) -> dict:
    """Load matching scores from CSV."""
    scores = {}
    with open(path) as f:
        reader = csv.DictReader(f)
        for row in reader:
            key = (row["image_a"], row["image_b"])
            scores[key] = float(row["score"])
    return scores


def load_summary(path: str) -> dict:
    """Load summary JSON."""
    with open(path) as f:
        return json.load(f)


def load_templates(templates_dir: str) -> dict:
    """Load all template .npy files from a directory.

    Handles both Python format ({name}_code.npy with shape 2,16,256,2)
    and C++ format ({name}_code_0.npy, {name}_code_1.npy with shape 16,512 each).
    """
    templates = {}
    dir_path = Path(templates_dir)
    if not dir_path.exists():
        return templates

    # Try Python format first: {name}_code.npy
    for f in sorted(dir_path.glob("*_code.npy")):
        # Skip C++ indexed files like _code_0.npy
        stem = f.stem
        if stem[-1].isdigit() and stem[-2] == "_":
            continue
        name = stem.replace("_code", "")
        code = np.load(str(f))
        mask_path = dir_path / f"{name}_mask.npy"
        if mask_path.exists():
            mask = np.load(str(mask_path))
            templates[name] = {"code": code, "mask": mask}

    if templates:
        return templates

    # Try C++ format: {name}_code_0.npy, {name}_code_1.npy
    seen_names = set()
    for f in sorted(dir_path.glob("*_code_0.npy")):
        name = f.stem.replace("_code_0", "")
        if name in seen_names:
            continue
        seen_names.add(name)

        code_0 = np.load(str(f))  # shape (16, 512)
        mask_0_path = dir_path / f"{name}_mask_0.npy"
        code_1_path = dir_path / f"{name}_code_1.npy"
        mask_1_path = dir_path / f"{name}_mask_1.npy"

        if not all(p.exists() for p in [mask_0_path, code_1_path, mask_1_path]):
            continue

        code_1 = np.load(str(code_1_path))
        mask_0 = np.load(str(mask_0_path))
        mask_1 = np.load(str(mask_1_path))

        # Reshape from (16, 512) to (16, 256, 2) and stack to (2, 16, 256, 2)
        # C++ stores real/imag interleaved: [r0, i0, r1, i1, ...]
        code = np.stack([code_0.reshape(code_0.shape[0], -1, 2),
                         code_1.reshape(code_1.shape[0], -1, 2)])
        mask = np.stack([mask_0.reshape(mask_0.shape[0], -1, 2),
                         mask_1.reshape(mask_1.shape[0], -1, 2)])
        templates[name] = {"code": code, "mask": mask}

    return templates


def compare_templates(py_templates: dict, cpp_templates: dict) -> dict:
    """Compare iris code templates between Python and C++."""
    common = set(py_templates.keys()) & set(cpp_templates.keys())
    if not common:
        return {"error": "no common templates found", "common_count": 0}

    bit_agreements = []
    mask_agreements = []

    for name in sorted(common):
        py_code = py_templates[name]["code"].astype(bool).flatten()
        cpp_code = cpp_templates[name]["code"].astype(bool).flatten()

        py_mask = py_templates[name]["mask"].astype(bool).flatten()
        cpp_mask = cpp_templates[name]["mask"].astype(bool).flatten()

        # Bit agreement (where both masks are valid)
        if len(py_code) == len(cpp_code):
            joint_mask = py_mask & cpp_mask
            if joint_mask.sum() > 0:
                agreement = np.mean(py_code[joint_mask] == cpp_code[joint_mask])
                bit_agreements.append(agreement)

            mask_agreement = np.mean(py_mask == cpp_mask)
            mask_agreements.append(mask_agreement)

    return {
        "common_templates": len(common),
        "py_only": len(py_templates) - len(common),
        "cpp_only": len(cpp_templates) - len(common),
        "bit_agreement": {
            "mean": float(np.mean(bit_agreements)) if bit_agreements else 0,
            "std": float(np.std(bit_agreements)) if bit_agreements else 0,
            "min": float(np.min(bit_agreements)) if bit_agreements else 0,
            "max": float(np.max(bit_agreements)) if bit_agreements else 0,
            "count": len(bit_agreements),
        },
        "mask_agreement": {
            "mean": float(np.mean(mask_agreements)) if mask_agreements else 0,
            "std": float(np.std(mask_agreements)) if mask_agreements else 0,
            "count": len(mask_agreements),
        },
    }


def compare_scores(py_scores: dict, cpp_scores: dict, label: str) -> dict:
    """Compare matching scores between Python and C++."""
    common_keys = set(py_scores.keys()) & set(cpp_scores.keys())
    if not common_keys:
        return {"error": f"no common {label} pairs", "common_count": 0}

    py_vals = np.array([py_scores[k] for k in sorted(common_keys)])
    cpp_vals = np.array([cpp_scores[k] for k in sorted(common_keys)])

    diffs = np.abs(py_vals - cpp_vals)
    correlation = float(np.corrcoef(py_vals, cpp_vals)[0, 1]) if len(py_vals) > 1 else 0

    return {
        "common_pairs": len(common_keys),
        "py_only": len(py_scores) - len(common_keys),
        "cpp_only": len(cpp_scores) - len(common_keys),
        "correlation": correlation,
        "score_diff": {
            "mean": float(diffs.mean()),
            "std": float(diffs.std()),
            "max": float(diffs.max()),
            "median": float(np.median(diffs)),
            "pct_within_0001": float(np.mean(diffs < 0.001) * 100),
            "pct_within_001": float(np.mean(diffs < 0.01) * 100),
            "pct_within_01": float(np.mean(diffs < 0.1) * 100),
        },
        "py_mean": float(py_vals.mean()),
        "cpp_mean": float(cpp_vals.mean()),
    }


def compute_roc_from_scores(genuine_path: str, impostor_path: str) -> dict:
    """Compute ROC metrics from CSV score files."""
    gen_scores = load_csv_scores(genuine_path)
    imp_scores = load_csv_scores(impostor_path)

    gen = np.array(list(gen_scores.values()))
    imp = np.array(list(imp_scores.values()))

    if len(gen) == 0 or len(imp) == 0:
        return {"error": "insufficient data"}

    fmr_thresholds = [0.1, 0.01, 0.001, 0.0001]
    metrics = {}

    imp_sorted = np.sort(imp)
    for fmr_target in fmr_thresholds:
        idx = int(len(imp_sorted) * fmr_target)
        threshold = imp_sorted[max(idx - 1, 0)]
        fnmr = float(np.mean(gen > threshold))
        metrics[f"FNMR@FMR={fmr_target}"] = fnmr

    # EER
    thresholds = np.linspace(0, 1, 10000)
    best_diff = 1.0
    eer_val = 0.5
    for t in thresholds:
        fmr = float(np.mean(imp <= t))
        fnmr = float(np.mean(gen > t))
        if abs(fmr - fnmr) < best_diff:
            best_diff = abs(fmr - fnmr)
            eer_val = (fmr + fnmr) / 2

    metrics["EER"] = eer_val
    return metrics


def generate_report(py_dir: str, cpp_dir: str, output_path: str):
    """Generate the full comparison report."""
    py_summary = load_summary(os.path.join(py_dir, "summary.json"))

    has_cpp = os.path.exists(os.path.join(cpp_dir, "summary.json"))
    cpp_summary = load_summary(os.path.join(cpp_dir, "summary.json")) if has_cpp else None

    # Load scores
    py_genuine = load_csv_scores(os.path.join(py_dir, "genuine_scores.csv"))
    py_impostor = load_csv_scores(os.path.join(py_dir, "impostor_scores.csv"))

    cpp_genuine = load_csv_scores(os.path.join(cpp_dir, "genuine_scores.csv")) if has_cpp else {}
    cpp_impostor = load_csv_scores(os.path.join(cpp_dir, "impostor_scores.csv")) if has_cpp else {}

    # Load templates for bit comparison
    py_templates = load_templates(os.path.join(py_dir, "templates"))
    cpp_templates = load_templates(os.path.join(cpp_dir, "templates")) if has_cpp else {}

    # Compute comparisons
    template_comparison = compare_templates(py_templates, cpp_templates) if cpp_templates else None
    genuine_comparison = compare_scores(py_genuine, cpp_genuine, "genuine") if cpp_genuine else None
    impostor_comparison = compare_scores(py_impostor, cpp_impostor, "impostor") if cpp_impostor else None

    # ROC for each
    py_roc = compute_roc_from_scores(
        os.path.join(py_dir, "genuine_scores.csv"),
        os.path.join(py_dir, "impostor_scores.csv"))
    cpp_roc = compute_roc_from_scores(
        os.path.join(cpp_dir, "genuine_scores.csv"),
        os.path.join(cpp_dir, "impostor_scores.csv")) if has_cpp else None

    # Build report
    lines = []
    lines.append("# Cross-Language Comparison Report: C++ vs Python")
    lines.append("")
    lines.append(f"**Dataset:** CASIA1 (108 subjects, 756 images)")
    lines.append(f"**Python:** open-iris v{py_summary.get('pipeline', 'unknown')}")
    if cpp_summary:
        lines.append(f"**C++:** {cpp_summary.get('pipeline', 'libiris')}")
    lines.append("")

    # --- Pipeline Success Rate ---
    lines.append("## 1. Pipeline Success Rate")
    lines.append("")
    lines.append("| Metric | Python | C++ |")
    lines.append("|--------|--------|-----|")
    py_ok = py_summary["successful"]
    py_fail = py_summary["failed"]
    py_total = py_summary["total_images"]
    cpp_ok = cpp_summary["successful"] if cpp_summary else "—"
    cpp_fail = cpp_summary["failed"] if cpp_summary else "—"
    cpp_total = cpp_summary["total_images"] if cpp_summary else "—"
    lines.append(f"| Total images | {py_total} | {cpp_total} |")
    lines.append(f"| Successful | {py_ok} ({py_ok/py_total*100:.1f}%) | {cpp_ok} |")
    lines.append(f"| Failed | {py_fail} | {cpp_fail} |")
    lines.append("")

    # --- Timing ---
    lines.append("## 2. Processing Speed")
    lines.append("")
    lines.append("| Metric | Python | C++ | Speedup |")
    lines.append("|--------|--------|-----|---------|")
    py_t = py_summary["timing"]
    if cpp_summary:
        cpp_t = cpp_summary["timing"]
        speedup = py_t["mean_ms"] / cpp_t["mean_ms"] if cpp_t["mean_ms"] > 0 else 0
        lines.append(f"| Mean (ms/image) | {py_t['mean_ms']:.1f} | {cpp_t['mean_ms']:.1f} | {speedup:.1f}x |")
        lines.append(f"| Median (ms/image) | {py_t['median_ms']:.1f} | {cpp_t['median_ms']:.1f} | {py_t['median_ms']/cpp_t['median_ms']:.1f}x |")
        lines.append(f"| Std dev | {py_t['std_ms']:.1f} | {cpp_t['std_ms']:.1f} | — |")
        lines.append(f"| Total (s) | {py_t['total_s']:.1f} | {cpp_t['total_s']:.1f} | {speedup:.1f}x |")
    else:
        lines.append(f"| Mean (ms/image) | {py_t['mean_ms']:.1f} | — | — |")
        lines.append(f"| Median (ms/image) | {py_t['median_ms']:.1f} | — | — |")
        lines.append(f"| Std dev | {py_t['std_ms']:.1f} | — | — |")
        lines.append(f"| Total (s) | {py_t['total_s']:.1f} | — | — |")
    lines.append("")

    # --- Matching Score Distributions ---
    lines.append("## 3. Matching Score Distributions")
    lines.append("")
    py_g = py_summary["genuine_scores"]
    py_i = py_summary["impostor_scores"]

    # Compute C++ score stats from CSV
    cpp_g_stats = {"mean": "—", "std": "—", "median": "—"}
    cpp_i_stats = {"mean": "—", "std": "—", "median": "—"}
    cpp_dprime = "—"
    py_dprime = "—"

    if cpp_genuine:
        gen_vals = np.array(list(cpp_genuine.values()))
        cpp_g_stats = {"mean": f"{gen_vals.mean():.4f}", "std": f"{gen_vals.std():.4f}",
                       "median": f"{np.median(gen_vals):.4f}"}
    if cpp_impostor:
        imp_vals = np.array(list(cpp_impostor.values()))
        cpp_i_stats = {"mean": f"{imp_vals.mean():.4f}", "std": f"{imp_vals.std():.4f}",
                       "median": f"{np.median(imp_vals):.4f}"}
    if cpp_genuine and cpp_impostor:
        gen_vals = np.array(list(cpp_genuine.values()))
        imp_vals = np.array(list(cpp_impostor.values()))
        pooled_std = np.sqrt((gen_vals.std()**2 + imp_vals.std()**2) / 2)
        if pooled_std > 0:
            cpp_dprime = f"{abs(imp_vals.mean() - gen_vals.mean()) / pooled_std:.2f}"

    py_pooled = np.sqrt((py_g['std']**2 + py_i['std']**2) / 2)
    if py_pooled > 0:
        py_dprime = f"{abs(py_i['mean'] - py_g['mean']) / py_pooled:.2f}"

    lines.append("| Metric | Python | C++ |")
    lines.append("|--------|--------|-----|")
    lines.append(f"| Genuine pairs | {py_g['count']} | {cpp_summary.get('genuine_count', '—') if cpp_summary else '—'} |")
    lines.append(f"| Genuine HD mean | {py_g['mean']:.4f} | {cpp_g_stats['mean']} |")
    lines.append(f"| Genuine HD std | {py_g['std']:.4f} | {cpp_g_stats['std']} |")
    lines.append(f"| Genuine HD median | {py_g['median']:.4f} | {cpp_g_stats['median']} |")
    lines.append(f"| Impostor pairs | {py_i['count']} | {cpp_summary.get('impostor_count', '—') if cpp_summary else '—'} |")
    lines.append(f"| Impostor HD mean | {py_i['mean']:.4f} | {cpp_i_stats['mean']} |")
    lines.append(f"| Impostor HD std | {py_i['std']:.4f} | {cpp_i_stats['std']} |")
    lines.append(f"| Impostor HD median | {py_i['median']:.4f} | {cpp_i_stats['median']} |")
    lines.append(f"| d' (separability) | {py_dprime} | {cpp_dprime} |")
    lines.append("")

    # --- ROC Metrics ---
    lines.append("## 4. ROC Metrics (Biometric Performance)")
    lines.append("")
    lines.append("| Metric | Python | C++ |")
    lines.append("|--------|--------|-----|")
    for key in ["EER", "FNMR@FMR=0.01", "FNMR@FMR=0.001", "FNMR@FMR=0.0001"]:
        py_val = py_roc.get(key, "—")
        cpp_val = cpp_roc.get(key, "—") if cpp_roc else "—"
        if isinstance(py_val, float):
            py_str = f"{py_val*100:.4f}%"
        else:
            py_str = str(py_val)
        if isinstance(cpp_val, float):
            cpp_str = f"{cpp_val*100:.4f}%"
        else:
            cpp_str = str(cpp_val)
        lines.append(f"| {key} | {py_str} | {cpp_str} |")
    lines.append("")

    # --- Cross-Language Agreement ---
    if template_comparison and "error" not in template_comparison:
        lines.append("## 5. Cross-Language Agreement")
        lines.append("")
        tc = template_comparison
        lines.append(f"**Common templates:** {tc['common_templates']} "
                      f"(Python-only: {tc['py_only']}, C++-only: {tc['cpp_only']})")
        lines.append("")
        ba = tc["bit_agreement"]
        ma = tc["mask_agreement"]
        lines.append("| Metric | Value |")
        lines.append("|--------|-------|")
        lines.append(f"| Iris code bit agreement | {ba['mean']*100:.2f}% ± {ba['std']*100:.2f}% |")
        lines.append(f"| Mask code agreement | {ma['mean']*100:.2f}% ± {ma['std']*100:.2f}% |")
        lines.append(f"| Min bit agreement | {ba['min']*100:.2f}% |")
        lines.append(f"| Max bit agreement | {ba['max']*100:.2f}% |")
        lines.append("")

    if genuine_comparison and "error" not in genuine_comparison:
        gc = genuine_comparison
        lines.append("### Genuine Score Agreement")
        lines.append("")
        lines.append(f"- **Correlation:** {gc['correlation']:.6f}")
        lines.append(f"- **Mean score diff:** {gc['score_diff']['mean']:.6f}")
        lines.append(f"- **Max score diff:** {gc['score_diff']['max']:.6f}")
        lines.append(f"- **Within 0.001:** {gc['score_diff']['pct_within_0001']:.1f}%")
        lines.append(f"- **Within 0.01:** {gc['score_diff']['pct_within_001']:.1f}%")
        lines.append("")

    if impostor_comparison and "error" not in impostor_comparison:
        ic = impostor_comparison
        lines.append("### Impostor Score Agreement")
        lines.append("")
        lines.append(f"- **Correlation:** {ic['correlation']:.6f}")
        lines.append(f"- **Mean score diff:** {ic['score_diff']['mean']:.6f}")
        lines.append(f"- **Max score diff:** {ic['score_diff']['max']:.6f}")
        lines.append(f"- **Within 0.001:** {ic['score_diff']['pct_within_0001']:.1f}%")
        lines.append(f"- **Within 0.01:** {ic['score_diff']['pct_within_001']:.1f}%")
        lines.append("")

    # --- Failures ---
    if py_summary.get("failures"):
        lines.append("## 6. Pipeline Failures")
        lines.append("")
        lines.append(f"**Python failures:** {len(py_summary['failures'])}")
        lines.append("")
        # Group by error type
        error_types = {}
        for f in py_summary["failures"]:
            err = f.get("error", "unknown")
            error_type = err.split(":")[0] if ":" in err else err.split(",")[0][:50]
            error_types[error_type] = error_types.get(error_type, 0) + 1
        lines.append("| Error Type | Count |")
        lines.append("|------------|-------|")
        for et, count in sorted(error_types.items(), key=lambda x: -x[1]):
            lines.append(f"| {et[:60]} | {count} |")
        lines.append("")

    # --- Conclusion ---
    lines.append("## Conclusion")
    lines.append("")
    if not has_cpp:
        lines.append("**C++ results not yet available.** Python baseline completed successfully.")
        lines.append(f"The Python pipeline processed {py_ok}/{py_total} images ({py_ok/py_total*100:.1f}% success rate).")
        lines.append("")
        lines.append(f"Genuine HD distribution: {py_g['mean']:.4f} ± {py_g['std']:.4f} (clear separation from impostors)")
        lines.append(f"Impostor HD distribution: {py_i['mean']:.4f} ± {py_i['std']:.4f}")
        if "EER" in py_roc:
            lines.append(f"EER: {py_roc['EER']*100:.2f}%")
    else:
        lines.append("Both pipelines have been evaluated on CASIA1.")
        if template_comparison and "error" not in template_comparison:
            ba = template_comparison["bit_agreement"]
            if ba["mean"] > 0.99:
                lines.append(f"Iris code bit agreement: **{ba['mean']*100:.2f}%** — near-identical output.")
            elif ba["mean"] > 0.95:
                lines.append(f"Iris code bit agreement: **{ba['mean']*100:.2f}%** — minor floating-point differences.")
            else:
                lines.append(f"Iris code bit agreement: **{ba['mean']*100:.2f}%** — significant differences detected.")
    lines.append("")

    report = "\n".join(lines)

    with open(output_path, "w") as f:
        f.write(report)

    print(report)
    print(f"\nReport saved to: {output_path}")


def main():
    base_dir = sys.argv[1] if len(sys.argv) > 1 else "/Users/innox/projects/iris2/iriscpp/biometric/iris/comparison"
    py_dir = os.path.join(base_dir, "python_results")
    cpp_dir = os.path.join(base_dir, "cpp_results")
    output_path = os.path.join(base_dir, "COMPARISON_REPORT.md")

    generate_report(py_dir, cpp_dir, output_path)


if __name__ == "__main__":
    main()
