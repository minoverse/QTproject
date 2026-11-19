#!/usr/bin/env python3

from pathlib import Path
import argparse
import csv
import re
import subprocess
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.patches import Patch


ROOT = Path(__file__).resolve().parents[1]

NOCUT_ROOT = (
    ROOT
    / "chaos_sweep/results/chaos_grid_a2_-10_10_nu_-0.1_0.9"
)

NOCUT_K = NOCUT_ROOT / "K_map.dat"
NOCUT_SERIES = NOCUT_ROOT / "series"

CUTS = {
    "A": {
        "k": ROOT / "dynamic_analysis/results/kmap_cut_A_c3eec1e507/K_map_skip_5000.dat",
        "series": ROOT / "dynamic_analysis/results/kmap_cut_A_c3eec1e507/series",
    },
    "B": {
        "k": ROOT / "dynamic_analysis/results/kmap_cut_B_8b68478d99/K_map_skip_5000.dat",
        "series": ROOT / "dynamic_analysis/results/kmap_cut_B_8b68478d99/series",
    },
    "C": {
        "k": ROOT / "dynamic_analysis/results/kmap_cut_C_6e392b4adc/K_map_skip_5000.dat",
        "series": ROOT / "dynamic_analysis/results/kmap_cut_C_6e392b4adc/series",
    },
    "D": {
        "k": ROOT / "dynamic_analysis/results/kmap_cut_D_4329eb2081/K_map_skip_5000.dat",
        "series": ROOT / "dynamic_analysis/results/kmap_cut_D_4329eb2081/series",
    },
    "E": {
        "k": ROOT / "dynamic_analysis/results/kmap_cut_E_739a4dd2da/K_map_skip_5000.dat",
        "series": ROOT / "dynamic_analysis/results/kmap_cut_E_739a4dd2da/series",
    },
}

OUT = ROOT / "dynamic_analysis/results/cut_comparison_analysis"
UPLOAD = OUT / "UPLOAD_TO_OVERLEAF"

CASE_RE = re.compile(r"^a2_(.+)_nu_(.+)$")


parser = argparse.ArgumentParser()

parser.add_argument("--skip", type=int, default=5000)
parser.add_argument("--tail", type=int, default=1000)

# 0-1 classification thresholds
parser.add_argument("--chaos-k", type=float, default=0.7)
parser.add_argument("--regular-k", type=float, default=0.3)

# periodicity detection
parser.add_argument("--max-period", type=int, default=16)
parser.add_argument("--period-tol", type=float, default=1e-4)
parser.add_argument("--fixed-tol", type=float, default=1e-5)

args = parser.parse_args()

OUT.mkdir(parents=True, exist_ok=True)
UPLOAD.mkdir(parents=True, exist_ok=True)


# =========================================================
# Helpers
# =========================================================

def key(a, n):
    return (round(float(a), 8), round(float(n), 8))


def fmt_num(x):
    if abs(x - round(x)) < 1e-9:
        return str(int(round(x)))
    return f"{x:g}"


def load_kmap(path):
    if not path.exists():
        raise SystemExit(f"Missing K-map: {path}")

    data = np.loadtxt(path)

    if data.ndim != 2 or data.shape[1] < 3:
        raise SystemExit(
            f"Expected alpha2 nu K columns: {path}"
        )

    out = {}

    for row in data:
        a = float(row[0])
        n = float(row[1])
        kvalue = float(row[2])

        if not np.isfinite(a):
            continue
        if not np.isfinite(n):
            continue
        if not np.isfinite(kvalue):
            continue

        # Correct range only
        if n < 0.1 - 1e-9 or n > 0.9 + 1e-9:
            continue

        out[key(a, n)] = kvalue

    return out


def build_series_index(series_root):
    index = {}

    if not series_root.exists():
        print(f"WARNING: series folder missing: {series_root}")
        return index

    for result in series_root.glob("a2_*_nu_*/result.dat"):

        m = CASE_RE.match(result.parent.name)

        if not m:
            continue

        try:
            a = float(m.group(1))
            n = float(m.group(2))
        except ValueError:
            continue

        index[key(a, n)] = result

    return index


def load_y1(path):
    # Only read y1, and skip the transient part.
    # Much faster than loading all five columns.
    try:
        x = np.loadtxt(
            path,
            usecols=(0,),
            skiprows=args.skip
        )
    except Exception:
        return None

    x = np.asarray(x, dtype=float)

    if x.ndim != 1:
        x = x.ravel()

    x = x[np.isfinite(x)]

    if len(x) == 0:
        return None

    return x


def detect_period(x):
    if x is None or len(x) < 100:
        return None, None

    tail_len = min(args.tail, len(x))
    y = np.asarray(x[-tail_len:], dtype=float)

    scale = max(
        1.0,
        float(np.max(np.abs(y)))
    )

    # Fixed point
    amplitude = float(np.ptp(y)) / scale

    if amplitude <= args.fixed_tol:
        return 1, amplitude

    # Test periods 2 ... max_period
    for p in range(2, args.max_period + 1):

        if len(y) < 10 * p:
            continue

        d = y[p:] - y[:-p]

        err = (
            float(np.sqrt(np.mean(d * d)))
            / scale
        )

        if err <= args.period_tol:
            return p, err

    return None, None


def classify(kvalue, result_path):

    if kvalue >= args.chaos_k:
        return "Chaotic", None, None

    if kvalue > args.regular_k:
        return "Transitional", None, None

    # K close to zero, including small negative K:
    # inspect raw y1 to distinguish fixed/periodic.
    if result_path is None:
        return "Regular/Unresolved", None, None

    x = load_y1(result_path)

    period, error = detect_period(x)

    if period == 1:
        return "Fixed", 1, error

    if period is not None:
        return "Periodic", period, error

    return "Regular/Unresolved", None, None


# =========================================================
# Load K data and series indices
# =========================================================

print("Loading existing K results...")

nocut_k = load_kmap(NOCUT_K)
nocut_series = build_series_index(NOCUT_SERIES)

cut_k = {}
cut_series = {}

for cut, info in CUTS.items():
    cut_k[cut] = load_kmap(info["k"])
    cut_series[cut] = build_series_index(info["series"])

print(f"No-cut K points: {len(nocut_k)}")

for cut in CUTS:
    print(
        f"Cut {cut}: "
        f"K={len(cut_k[cut])}, "
        f"raw series={len(cut_series[cut])}"
    )


# =========================================================
# 1. Delta-K maps A-E
#
# deltaK = K_cut - K_no_cut
#
# each point:
#   color = deltaK
#   printed number = actual deltaK
# =========================================================

delta_records = {}

for cut in CUTS:

    records = []

    common = sorted(
        set(nocut_k.keys())
        & set(cut_k[cut].keys()),
        key=lambda z: (z[1], z[0])
    )

    for a, n in common:

        k0 = nocut_k[(a, n)]
        kc = cut_k[cut][(a, n)]

        dk = kc - k0

        records.append(
            {
                "alpha2": a,
                "nu": n,
                "K_nocut": k0,
                "K_cut": kc,
                "deltaK": dk,
            }
        )

    delta_records[cut] = records

    # CSV
    csv_path = OUT / f"deltaK_cut_{cut}.csv"

    with open(csv_path, "w", newline="") as f:

        writer = csv.DictWriter(
            f,
            fieldnames=[
                "alpha2",
                "nu",
                "K_nocut",
                "K_cut",
                "deltaK",
            ]
        )

        writer.writeheader()
        writer.writerows(records)

    a = np.array(
        [r["alpha2"] for r in records]
    )

    n = np.array(
        [r["nu"] for r in records]
    )

    dk = np.array(
        [r["deltaK"] for r in records]
    )

    # ---------------------------------------------
    # Large value + color map
    # ---------------------------------------------

    fig, ax = plt.subplots(
        figsize=(21, 9)
    )

    # Raw value preserved in CSV/text.
    # Color saturates outside [-1, 1].
    display_dk = np.clip(dk, -1.0, 1.0)

    sc = ax.scatter(
        a,
        n,
        c=display_dk,
        cmap="coolwarm",
        vmin=-1.1,
        vmax=1.1,
        marker="s",
        s=720
    )

    # Value printed on every point
    for x, y, value in zip(a, n, dk):

        text_color = (
            "white"
            if abs(value) > 0.45
            else "black"
        )

        ax.text(
            x,
            y,
            f"{value:+.2f}",
            ha="center",
            va="center",
            fontsize=7,
            color=text_color
        )

    cb = fig.colorbar(sc, ax=ax)

    cb.set_label(
        r"$\Delta K = K_{cut} - K_{no\ cut}$"
    )

    ax.set_title(
        f"Cut {cut} vs No Cut: Delta K"
    )

    ax.set_xlabel(r"$\alpha_2$")
    ax.set_ylabel(r"$\nu$")

    ax.set_xticks(range(-10, 11))

    ax.set_yticks(
        [
            0.1, 0.2, 0.3,
            0.4, 0.5, 0.6,
            0.7, 0.8, 0.9
        ]
    )

    ax.set_xlim(-10.5, 10.5)
    ax.set_ylim(0.05, 0.95)

    ax.grid(True, alpha=0.2)

    fig.tight_layout()

    png = (
        UPLOAD
        / f"deltaK_cut_{cut}_vs_nocut.png"
    )

    fig.savefig(
        png,
        dpi=180,
        bbox_inches="tight"
    )

    plt.close(fig)

    print(
        f"Delta K Cut {cut}: "
        f"{len(records)} points -> {png}"
    )


# =========================================================
# 2. Fixed / Periodic / Chaotic classification
# =========================================================

datasets = {
    "NoCut": (nocut_k, nocut_series),
}

for cut in CUTS:
    datasets[f"Cut_{cut}"] = (
        cut_k[cut],
        cut_series[cut]
    )


class_colors = {
    "Fixed": "tab:blue",
    "Periodic": "tab:green",
    "Chaotic": "tab:red",
    "Regular/Unresolved": "tab:gray",
    "Transitional": "tab:orange",
}


classification_all = {}


for name, (kmap, series_index) in datasets.items():

    print(f"Classifying {name}...")

    records = []

    for (a, n), kvalue in sorted(
        kmap.items(),
        key=lambda z: (z[0][1], z[0][0])
    ):

        result_file = series_index.get(
            key(a, n)
        )

        state, period, error = classify(
            kvalue,
            result_file
        )

        fft_needed = ""

        if state == "Fixed":
            fft_needed = "NO"

        elif state == "Periodic" and period is not None:
            fft_needed = (
                "NO"
                if period < 8
                else "YES"
            )

        records.append(
            {
                "dataset": name,
                "alpha2": a,
                "nu": n,
                "K": kvalue,
                "state": state,
                "period": (
                    ""
                    if period is None
                    else period
                ),
                "period_error": (
                    ""
                    if error is None
                    else error
                ),
                "period_less_than_8": (
                    ""
                    if period is None
                    else period < 8
                ),
                "fft_needed_if_prof_rule": fft_needed,
                "result_file": (
                    ""
                    if result_file is None
                    else str(result_file)
                ),
            }
        )

    classification_all[name] = records

    csv_path = (
        OUT
        / f"classification_{name}.csv"
    )

    with open(csv_path, "w", newline="") as f:

        writer = csv.DictWriter(
            f,
            fieldnames=records[0].keys()
        )

        writer.writeheader()
        writer.writerows(records)

    # Classification map
    fig, ax = plt.subplots(
        figsize=(13, 8)
    )

    for state, color in class_colors.items():

        subset = [
            r for r in records
            if r["state"] == state
        ]

        if not subset:
            continue

        ax.scatter(
            [r["alpha2"] for r in subset],
            [r["nu"] for r in subset],
            s=75,
            label=state,
            color=color
        )

    ax.set_title(
        f"{name}: Fixed / Periodic / Chaotic"
    )

    ax.set_xlabel(r"$\alpha_2$")
    ax.set_ylabel(r"$\nu$")

    ax.set_xticks(range(-10, 11))

    ax.set_yticks(
        [
            0.1, 0.2, 0.3,
            0.4, 0.5, 0.6,
            0.7, 0.8, 0.9
        ]
    )

    ax.set_xlim(-10.5, 10.5)
    ax.set_ylim(0.05, 0.95)

    ax.grid(True, alpha=0.25)
    ax.legend()

    fig.tight_layout()

    png = (
        UPLOAD
        / f"classification_{name}.png"
    )

    fig.savefig(
        png,
        dpi=180,
        bbox_inches="tight"
    )

    plt.close(fig)

    print(f"  -> {png}")


# =========================================================
# 3. Period / FFT-rule summary
# =========================================================

summary_path = OUT / "classification_summary.txt"

with open(summary_path, "w") as f:

    f.write(
        "Fixed / Periodic / Chaotic classification\n"
    )

    f.write(
        "=========================================\n\n"
    )

    f.write(
        f"K chaotic threshold = {args.chaos_k}\n"
    )

    f.write(
        f"K regular threshold = {args.regular_k}\n"
    )

    f.write(
        f"Maximum tested period = {args.max_period}\n"
    )

    f.write(
        "Professor rule being checked: "
        "period < 8 -> FFT not needed\n\n"
    )

    for name, records in classification_all.items():

        states = {}

        periods = {}

        for r in records:

            states[r["state"]] = (
                states.get(r["state"], 0)
                + 1
            )

            if r["period"] != "":
                p = int(r["period"])

                periods[p] = (
                    periods.get(p, 0)
                    + 1
                )

        f.write(f"[{name}]\n")

        for state in sorted(states):
            f.write(
                f"{state}: {states[state]}\n"
            )

        f.write(
            f"Detected periods: {periods}\n"
        )

        ge8 = sum(
            count
            for p, count in periods.items()
            if p >= 8
        )

        lt8 = sum(
            count
            for p, count in periods.items()
            if p < 8
        )

        f.write(
            f"Detected period < 8: {lt8}\n"
        )

        f.write(
            f"Detected period >= 8: {ge8}\n\n"
        )


# =========================================================
# 4. Cut A numerical validation
# =========================================================

cutA_records = delta_records["A"]

# Find strongest regular -> chaotic changes first
candidates = [
    r for r in cutA_records
    if (
        r["K_cut"] >= args.chaos_k
        and r["K_nocut"] <= args.regular_k
    )
]

candidates.sort(
    key=lambda r: r["deltaK"],
    reverse=True
)

# Choose up to 3 representative cases,
# preferably at different nu values.
selected = []
used_nu = set()

for r in candidates:

    nkey = round(r["nu"], 8)

    if nkey in used_nu:
        continue

    selected.append(r)
    used_nu.add(nkey)

    if len(selected) == 3:
        break

# Fill if fewer than 3
if len(selected) < 3:

    for r in candidates:

        if r in selected:
            continue

        selected.append(r)

        if len(selected) == 3:
            break


validation_csv = (
    OUT
    / "cut_A_validation_candidates.csv"
)

with open(validation_csv, "w", newline="") as f:

    writer = csv.DictWriter(
        f,
        fieldnames=[
            "alpha2",
            "nu",
            "K_nocut",
            "K_cut",
            "deltaK",
        ]
    )

    writer.writeheader()
    writer.writerows(selected)


# =========================================================
# 5. Professor P-Q program for Cut A representatives
# =========================================================

prof_src = ROOT / "chaos_sweep/prof_test01.c"
test_bin = OUT / "test01_validation"

can_run_pq = prof_src.exists()

if can_run_pq:

    source_text = prof_src.read_text(
        errors="ignore"
    )

    if "srand(time(0))" in source_text:
        print(
            "WARNING: prof_test01.c still uses srand(time(0)); "
            "P-Q selection will not be reproducible."
        )

    subprocess.run(
        [
            "gcc",
            "-O2",
            "-std=gnu11",
            str(prof_src),
            "-o",
            str(test_bin),
            "-lm",
        ],
        check=True
    )

else:
    print(
        f"WARNING: professor source not found: {prof_src}"
    )


validation_root = (
    OUT
    / "cut_A_validation"
)

validation_root.mkdir(
    parents=True,
    exist_ok=True
)


def run_prof_pq(result_file, out_dir):

    out_dir.mkdir(
        parents=True,
        exist_ok=True
    )

    proc = subprocess.run(
        [
            str(test_bin),
            str(args.skip),
            "pq",
            str(result_file),
        ],
        cwd=out_dir,
        text=True,
        capture_output=True
    )

    (
        out_dir / "test01_output.txt"
    ).write_text(
        proc.stdout
        + "\n"
        + proc.stderr
    )

    pq_file = out_dir / "pq.dat"

    if (
        proc.returncode != 0
        or not pq_file.exists()
    ):
        return None

    pq = np.loadtxt(pq_file)

    if pq.ndim != 2 or pq.shape[1] < 3:
        return None

    # Professor pq.dat:
    # col 0 = n
    # col 1 = p(n)
    # col 2 = q(n)
    return pq


for idx, r in enumerate(selected, start=1):

    a = r["alpha2"]
    n = r["nu"]

    nocut_file = nocut_series.get(
        key(a, n)
    )

    cutA_file = cut_series["A"].get(
        key(a, n)
    )

    if nocut_file is None or cutA_file is None:
        print(
            f"WARNING: raw file missing "
            f"a2={a}, nu={n}"
        )
        continue

    case_dir = (
        validation_root
        / f"case_{idx}_a2_{fmt_num(a)}_nu_{fmt_num(n)}"
    )

    case_dir.mkdir(
        parents=True,
        exist_ok=True
    )

    # ---------------------------------------------
    # Time-series comparison
    # ---------------------------------------------

    y0 = load_y1(nocut_file)
    ya = load_y1(cutA_file)

    if y0 is not None and ya is not None:

        show_n = min(
            1000,
            len(y0),
            len(ya)
        )

        fig, axes = plt.subplots(
            2,
            1,
            figsize=(12, 7),
            sharex=True
        )

        axes[0].plot(
            np.arange(show_n),
            y0[-show_n:],
            linewidth=0.8
        )

        axes[0].set_title(
            f"No Cut: alpha2={a:g}, nu={n:g}, "
            f"K={r['K_nocut']:.4f}"
        )

        axes[0].set_ylabel("y1")

        axes[1].plot(
            np.arange(show_n),
            ya[-show_n:],
            linewidth=0.8
        )

        axes[1].set_title(
            f"Cut A: alpha2={a:g}, nu={n:g}, "
            f"K={r['K_cut']:.4f}"
        )

        axes[1].set_xlabel(
            "Sample after transient"
        )

        axes[1].set_ylabel("y1")

        fig.tight_layout()

        time_png = (
            UPLOAD
            / (
                f"cut_A_validation_case_{idx}_"
                f"a2_{fmt_num(a)}_nu_{fmt_num(n)}_timeseries.png"
            )
        )

        fig.savefig(
            time_png,
            dpi=180,
            bbox_inches="tight"
        )

        plt.close(fig)

    # ---------------------------------------------
    # Professor P-Q comparison
    # ---------------------------------------------

    if can_run_pq:

        pq0 = run_prof_pq(
            nocut_file,
            case_dir / "nocut"
        )

        pqa = run_prof_pq(
            cutA_file,
            case_dir / "cutA"
        )

        if pq0 is not None and pqa is not None:

            fig, axes = plt.subplots(
                1,
                2,
                figsize=(12, 5)
            )

            # dots only
            axes[0].scatter(
                pq0[:, 1],
                pq0[:, 2],
                s=2
            )

            axes[0].set_title(
                f"No Cut P-Q\n"
                f"a2={a:g}, nu={n:g}, "
                f"K={r['K_nocut']:.4f}"
            )

            axes[0].set_xlabel("p")
            axes[0].set_ylabel("q")

            axes[1].scatter(
                pqa[:, 1],
                pqa[:, 2],
                s=2
            )

            axes[1].set_title(
                f"Cut A P-Q\n"
                f"a2={a:g}, nu={n:g}, "
                f"K={r['K_cut']:.4f}"
            )

            axes[1].set_xlabel("p")
            axes[1].set_ylabel("q")

            fig.tight_layout()

            pq_png = (
                UPLOAD
                / (
                    f"cut_A_validation_case_{idx}_"
                    f"a2_{fmt_num(a)}_nu_{fmt_num(n)}_PQ.png"
                )
            )

            fig.savefig(
                pq_png,
                dpi=180,
                bbox_inches="tight"
            )

            plt.close(fig)


# =========================================================
# 6. Cut A summary
# =========================================================

a_values = np.array(
    [r["K_cut"] for r in cutA_records]
)

a_delta = np.array(
    [r["deltaK"] for r in cutA_records]
)

a_chaotic = int(
    np.sum(
        a_values >= args.chaos_k
    )
)

regular_to_chaos = sum(
    1
    for r in cutA_records
    if (
        r["K_nocut"] <= args.regular_k
        and r["K_cut"] >= args.chaos_k
    )
)

cutA_summary = (
    OUT
    / "cut_A_summary.txt"
)

with open(cutA_summary, "w") as f:

    f.write("Cut A validation\n")
    f.write("================\n\n")

    f.write(
        f"Total comparable points: "
        f"{len(cutA_records)}\n"
    )

    f.write(
        f"Cut A chaotic points "
        f"(K >= {args.chaos_k}): "
        f"{a_chaotic}/{len(cutA_records)}\n"
    )

    f.write(
        f"No-cut regular -> Cut A chaotic: "
        f"{regular_to_chaos}\n"
    )

    f.write(
        f"Mean deltaK: "
        f"{np.mean(a_delta):+.6f}\n"
    )

    f.write(
        f"Median deltaK: "
        f"{np.median(a_delta):+.6f}\n"
    )

    f.write("\nRepresentative cases:\n")

    for i, r in enumerate(selected, start=1):

        f.write(
            f"{i}. "
            f"alpha2={r['alpha2']:g}, "
            f"nu={r['nu']:g}, "
            f"K_no={r['K_nocut']:.6f}, "
            f"K_cutA={r['K_cut']:.6f}, "
            f"deltaK={r['deltaK']:+.6f}\n"
        )


print("")
print("DONE")
print(f"Results: {OUT}")
print(f"Overleaf: {UPLOAD}")
print("")
print("Check these first:")
print(f"  {summary_path}")
print(f"  {cutA_summary}")
print(f"  {validation_csv}")
