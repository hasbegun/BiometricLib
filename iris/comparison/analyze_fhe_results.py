#!/usr/bin/env python3
"""
Analyze FHE comparison results.

Reads fhe_scores.csv from fhe_comparison tool and produces:
  - fhe_scatter.png         (plaintext_hd vs encrypted_hd)
  - fhe_timing.png          (encrypt/match/decrypt time box plots)
  - fhe_analysis.md         (markdown summary)

Usage:
    python analyze_fhe_results.py <fhe_scores.csv> [output_dir]
"""

import csv
import sys
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np


def main():
    if len(sys.argv) < 2:
        print("Usage: analyze_fhe_results.py <fhe_scores.csv> [output_dir]")
        sys.exit(1)

    csv_path = Path(sys.argv[1])
    output_dir = Path(sys.argv[2]) if len(sys.argv) > 2 else csv_path.parent

    output_dir.mkdir(parents=True, exist_ok=True)

    # Read CSV
    rows = []
    with open(csv_path) as f:
        reader = csv.DictReader(f)
        for row in reader:
            rows.append({
                "image_a": row["image_a"],
                "image_b": row["image_b"],
                "plaintext_hd": float(row["plaintext_hd"]),
                "encrypted_hd": float(row["encrypted_hd"]),
                "abs_diff": float(row["abs_diff"]),
                "encrypt_ms": float(row["encrypt_ms"]),
                "match_ms": float(row["match_ms"]),
                "decrypt_ms": float(row["decrypt_ms"]),
            })

    if not rows:
        print("No data in CSV")
        sys.exit(1)

    pt = np.array([r["plaintext_hd"] for r in rows])
    enc = np.array([r["encrypted_hd"] for r in rows])
    diffs = np.array([r["abs_diff"] for r in rows])
    encrypt_ms = np.array([r["encrypt_ms"] for r in rows])
    match_ms = np.array([r["match_ms"] for r in rows])
    decrypt_ms = np.array([r["decrypt_ms"] for r in rows])
    total_ms = encrypt_ms + match_ms + decrypt_ms

    # Decision agreement at threshold 0.35
    threshold = 0.35
    pt_match = pt < threshold
    enc_match = enc < threshold
    disagreements = int(np.sum(pt_match != enc_match))

    print(f"Pairs:              {len(rows)}")
    print(f"Max |pt - enc|:     {diffs.max():.2e}")
    print(f"Mean |pt - enc|:    {diffs.mean():.2e}")
    print(f"Diffs > 1e-10:      {(diffs > 1e-10).sum()}")
    print(f"Diffs > 1e-6:       {(diffs > 1e-6).sum()}")
    print(f"Disagreements@0.35: {disagreements}/{len(rows)}")

    # --- Scatter plot ---
    fig, ax = plt.subplots(1, 1, figsize=(7, 7))
    ax.scatter(pt, enc, s=8, alpha=0.5, c="steelblue")
    lims = [0, max(pt.max(), enc.max()) * 1.05]
    ax.plot(lims, lims, "--", color="red", linewidth=1, label="y=x (exact match)")
    ax.set_xlabel("Plaintext HD")
    ax.set_ylabel("Encrypted HD")
    ax.set_title(f"FHE vs Plaintext HD  (max diff={diffs.max():.2e}, n={len(rows)})")
    ax.legend()
    ax.grid(alpha=0.3)
    ax.set_aspect("equal")
    fig.tight_layout()
    fig.savefig(output_dir / "fhe_scatter.png", dpi=150, bbox_inches="tight")
    plt.close(fig)
    print(f"Saved: {output_dir / 'fhe_scatter.png'}")

    # --- Timing box plots ---
    fig, ax = plt.subplots(1, 1, figsize=(8, 5))
    data = [encrypt_ms, match_ms, decrypt_ms, total_ms]
    labels = ["Encrypt\n(2 templates)", "Match\n(encrypted)", "Decrypt\n(result)", "Total"]
    bp = ax.boxplot(data, labels=labels, patch_artist=True)
    colors = ["#66b2ff", "#99ff99", "#ff9999", "#ffcc66"]
    for patch, color in zip(bp["boxes"], colors):
        patch.set_facecolor(color)
    ax.set_ylabel("Time (ms)")
    ax.set_title("FHE Operation Timing")
    ax.grid(alpha=0.3, axis="y")
    fig.tight_layout()
    fig.savefig(output_dir / "fhe_timing.png", dpi=150, bbox_inches="tight")
    plt.close(fig)
    print(f"Saved: {output_dir / 'fhe_timing.png'}")

    # --- Markdown summary ---
    verdict = "PASS" if diffs.max() < 1e-6 and disagreements == 0 else "FAIL"
    md = f"""# FHE Verification Results

## Score Agreement

| Metric | Value |
|--------|-------|
| Pairs compared | {len(rows)} |
| Max |plaintext - encrypted| | {diffs.max():.12f} |
| Mean |plaintext - encrypted| | {diffs.mean():.12f} |
| Pairs with diff > 1e-10 | {(diffs > 1e-10).sum()} |
| Pairs with diff > 1e-6 | {(diffs > 1e-6).sum()} |
| Decision disagreements at T=0.35 | {disagreements}/{len(rows)} |

## Verdict: {verdict}

Encrypted matching produces {"bit-exact" if diffs.max() == 0 else "near-exact"} results compared to plaintext.

## Timing (per pair)

| Operation | Mean (ms) | Std (ms) | Min (ms) | Max (ms) |
|-----------|-----------|----------|----------|----------|
| Encrypt (2 templates) | {encrypt_ms.mean():.1f} | {encrypt_ms.std():.1f} | {encrypt_ms.min():.1f} | {encrypt_ms.max():.1f} |
| Match (encrypted) | {match_ms.mean():.1f} | {match_ms.std():.1f} | {match_ms.min():.1f} | {match_ms.max():.1f} |
| Decrypt (result) | {decrypt_ms.mean():.1f} | {decrypt_ms.std():.1f} | {decrypt_ms.min():.1f} | {decrypt_ms.max():.1f} |
| **Total** | **{total_ms.mean():.1f}** | **{total_ms.std():.1f}** | **{total_ms.min():.1f}** | **{total_ms.max():.1f}** |

## Visual Evidence

- [FHE Scatter Plot](fhe_scatter.png) — all points should lie on y=x
- [FHE Timing](fhe_timing.png) — per-operation timing distributions
"""

    md_path = output_dir / "fhe_analysis.md"
    md_path.write_text(md)
    print(f"Saved: {md_path}")


if __name__ == "__main__":
    main()
