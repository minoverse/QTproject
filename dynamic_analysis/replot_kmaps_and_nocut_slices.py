#!/usr/bin/env python3

import argparse
from pathlib import Path

import numpy as np
import matplotlib.pyplot as plt


ROOT = Path(__file__).resolve().parents[1]

CUT_FILES = {
    "A": ROOT / "dynamic_analysis/results/kmap_cut_A_c3eec1e507/K_map_skip_5000.dat",
    "B": ROOT / "dynamic_analysis/results/kmap_cut_B_8b68478d99/K_map_skip_5000.dat",
    "C": ROOT / "dynamic_analysis/results/kmap_cut_C_6e392b4adc/K_map_skip_5000.dat",
    "D": ROOT / "dynamic_analysis/results/kmap_cut_D_4329eb2081/K_map_skip_5000.dat",
    "E": ROOT / "dynamic_analysis/results/kmap_cut_E_739a4dd2da/K_map_skip_5000.dat",
}

NOCUT_FILE = (
    ROOT
    / "chaos_sweep/results/chaos_grid_a2_-10_10_nu_-0.1_0.9/K_map.dat"
)

OUT = ROOT / "dynamic_analysis/results/kmap_fixed_nu_0.1_0.9"
UPLOAD = OUT / "UPLOAD_TO_OVERLEAF"

parser = argparse.ArgumentParser()

parser.add_argument(
    "--size",
    type=float,
    default=55,
    help="K-map point size"
)

args = parser.parse_args()

OUT.mkdir(parents=True, exist_ok=True)
UPLOAD.mkdir(parents=True, exist_ok=True)


def load_kmap(path):

    if not path.exists():
        raise SystemExit(f"Missing file: {path}")

    data = np.loadtxt(path)

    if data.ndim != 2 or data.shape[1] < 3:
        raise SystemExit(f"Expected alpha2 nu K columns: {path}")

    alpha2 = data[:, 0]
    nu = data[:, 1]
    K = data[:, 2]

    valid = (
        np.isfinite(alpha2)
        & np.isfinite(nu)
        & np.isfinite(K)
    )

    return alpha2[valid], nu[valid], K[valid]


# =========================================================
# 1. Cut A-E K maps
#    only nu = 0.1 ... 0.9
# =========================================================

for cut, path in CUT_FILES.items():

    alpha2, nu, K = load_kmap(path)

    keep = (
        (nu >= 0.1 - 1e-12)
        & (nu <= 0.9 + 1e-12)
    )

    alpha2 = alpha2[keep]
    nu = nu[keep]
    K = K[keep]

    filtered_dat = OUT / f"K_map_cut_{cut}_nu_0.1_0.9.dat"

    np.savetxt(
        filtered_dat,
        np.column_stack((alpha2, nu, K)),
        fmt="%.10g",
        header="alpha2 nu K"
    )

    fig, ax = plt.subplots(figsize=(12, 8))

    sc = ax.scatter(
        alpha2,
        nu,
        c=K,
        s=args.size,
        cmap="inferno",
        vmin=0,
        vmax=1
    )

    cb = fig.colorbar(sc, ax=ax)
    cb.set_label("K")

    ax.set_xlabel(r"$\alpha_2$")
    ax.set_ylabel(r"$\nu$")
    ax.set_title(f"0-1 Chaos Test: Cut {cut}")

    ax.set_xlim(-10.5, 10.5)
    ax.set_ylim(0.05, 0.95)

    ax.set_xticks(range(-10, 11))
    ax.set_yticks(
        [0.1, 0.2, 0.3, 0.4, 0.5,
         0.6, 0.7, 0.8, 0.9]
    )

    ax.grid(True, alpha=0.25)

    fig.tight_layout()

    png = UPLOAD / f"kmap_cut_{cut}_nu_0.1_0.9.png"

    fig.savefig(
        png,
        dpi=180,
        bbox_inches="tight"
    )

    plt.close(fig)

    print(f"Cut {cut}: {len(K)} valid points -> {png}")


# =========================================================
# 2. No-cut K map
# =========================================================

alpha2, nu, K = load_kmap(NOCUT_FILE)

keep = (
    (nu >= 0.1 - 1e-12)
    & (nu <= 0.9 + 1e-12)
)

alpha2 = alpha2[keep]
nu = nu[keep]
K = K[keep]

fig, ax = plt.subplots(figsize=(12, 8))

sc = ax.scatter(
    alpha2,
    nu,
    c=K,
    s=args.size,
    cmap="inferno",
    vmin=0,
    vmax=1
)

cb = fig.colorbar(sc, ax=ax)
cb.set_label("K")

ax.set_xlabel(r"$\alpha_2$")
ax.set_ylabel(r"$\nu$")
ax.set_title("0-1 Chaos Test: No Cut")

ax.set_xlim(-10.5, 10.5)
ax.set_ylim(0.05, 0.95)

ax.set_xticks(range(-10, 11))
ax.set_yticks(
    [0.1, 0.2, 0.3, 0.4, 0.5,
     0.6, 0.7, 0.8, 0.9]
)

ax.grid(True, alpha=0.25)

fig.tight_layout()

nocut_png = UPLOAD / "kmap_nocut_nu_0.1_0.9.png"

fig.savefig(
    nocut_png,
    dpi=180,
    bbox_inches="tight"
)

plt.close(fig)

print(f"No-cut K-map -> {nocut_png}")


# =========================================================
# 3. No-cut slices
#
# x-axis = alpha2
# y-axis = K
#
# DOTS ONLY
# =========================================================

for target_nu in [0.4, 0.9]:

    select = np.isclose(
        nu,
        target_nu,
        atol=1e-9
    )

    a = alpha2[select]
    k = K[select]

    order = np.argsort(a)

    a = a[order]
    k = k[order]

    if len(a) == 0:
        print(f"WARNING: no points for nu={target_nu}")
        continue

    csv_path = OUT / f"nocut_nu_{target_nu:.1f}_slice.csv"

    np.savetxt(
        csv_path,
        np.column_stack((a, k)),
        delimiter=",",
        header="alpha2,K",
        comments="",
        fmt="%.10g"
    )

    fig, ax = plt.subplots(figsize=(11, 6))

    # DOTS ONLY
    ax.scatter(
        a,
        k,
        s=80
    )

    ax.set_xlabel(r"$\alpha_2$")
    ax.set_ylabel("K")

    ax.set_title(
        f"No Cut: K vs alpha2 at nu={target_nu:.1f}"
    )

    ax.set_xlim(-10.5, 10.5)
    ax.set_ylim(-0.12, 1.05)

    ax.set_xticks(range(-10, 11))
    ax.set_yticks(
        [-0.1, 0.0, 0.2, 0.4,
         0.6, 0.8, 1.0]
    )

    ax.grid(True, alpha=0.3)

    fig.tight_layout()

    png = (
        UPLOAD
        / f"nocut_nu_{target_nu:.1f}_K_vs_alpha2.png"
    )

    fig.savefig(
        png,
        dpi=180,
        bbox_inches="tight"
    )

    plt.close(fig)

    print(
        f"No-cut nu={target_nu:.1f}: "
        f"{len(a)} alpha2 points -> {png}"
    )


print("")
print("DONE")
print(f"Results: {OUT}")
print(f"Overleaf PNGs: {UPLOAD}")
