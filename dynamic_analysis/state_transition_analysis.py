#!/usr/bin/env python3

from pathlib import Path
import csv
from collections import Counter

import matplotlib.pyplot as plt
import numpy as np


ROOT = Path(__file__).resolve().parents[1]

INPUT = (
    ROOT
    / "dynamic_analysis/results/cut_comparison_analysis"
)

OUT = (
    ROOT
    / "dynamic_analysis/results/state_transition_analysis"
)

UPLOAD = OUT / "UPLOAD_TO_OVERLEAF"

OUT.mkdir(parents=True, exist_ok=True)
UPLOAD.mkdir(parents=True, exist_ok=True)


FILES = {
    "NoCut": INPUT / "classification_NoCut.csv",
    "A": INPUT / "classification_Cut_A.csv",
    "B": INPUT / "classification_Cut_B.csv",
    "C": INPUT / "classification_Cut_C.csv",
    "D": INPUT / "classification_Cut_D.csv",
    "E": INPUT / "classification_Cut_E.csv",
}


CORE_STATES = [
    "Fixed",
    "Periodic",
    "Chaotic",
]


def key(alpha2, nu):
    return (
        round(float(alpha2), 8),
        round(float(nu), 8),
    )


def load_classification(path):

    if not path.exists():
        raise SystemExit(
            f"Missing classification file: {path}"
        )

    data = {}

    with open(path, newline="") as f:

        reader = csv.DictReader(f)

        for row in reader:

            a = float(row["alpha2"])
            n = float(row["nu"])

            # Use only correct nu range
            if n < 0.1 - 1e-9:
                continue

            if n > 0.9 + 1e-9:
                continue

            data[key(a, n)] = {
                "alpha2": a,
                "nu": n,
                "K": float(row["K"]),
                "state": row["state"],
                "period": row["period"],
            }

    return data


print("Loading classification results...")

datasets = {}

for name, path in FILES.items():

    datasets[name] = load_classification(path)

    print(
        f"{name}: {len(datasets[name])} points"
    )


nocut = datasets["NoCut"]


# =========================================================
# Transition appearance
# =========================================================

TRANSITION_STYLE = {
    "Fixed -> Fixed":
        ("tab:blue", "o"),

    "Fixed -> Periodic":
        ("tab:cyan", "o"),

    "Fixed -> Chaotic":
        ("tab:purple", "o"),

    "Periodic -> Fixed":
        ("tab:olive", "s"),

    "Periodic -> Periodic":
        ("tab:green", "s"),

    "Periodic -> Chaotic":
        ("tab:red", "s"),

    "Chaotic -> Fixed":
        ("tab:brown", "^"),

    "Chaotic -> Periodic":
        ("tab:pink", "^"),

    "Chaotic -> Chaotic":
        ("tab:orange", "^"),

    "Other":
        ("tab:gray", "x"),
}


all_summary_rows = []


# =========================================================
# Analyze Cut A-E
# =========================================================

for cut in ["A", "B", "C", "D", "E"]:

    cut_data = datasets[cut]

    common = sorted(
        set(nocut.keys()) & set(cut_data.keys()),
        key=lambda x: (x[1], x[0])
    )

    records = []

    transition_counter = Counter()
    other_counter = Counter()

    for point in common:

        no = nocut[point]
        cu = cut_data[point]

        from_state = no["state"]
        to_state = cu["state"]

        if (
            from_state in CORE_STATES
            and to_state in CORE_STATES
        ):

            transition = (
                f"{from_state} -> {to_state}"
            )

        else:

            transition = "Other"

            other_counter[
                f"{from_state} -> {to_state}"
            ] += 1

        transition_counter[transition] += 1

        records.append(
            {
                "alpha2": no["alpha2"],
                "nu": no["nu"],
                "K_nocut": no["K"],
                "K_cut": cu["K"],
                "state_nocut": from_state,
                "state_cut": to_state,
                "period_nocut": no["period"],
                "period_cut": cu["period"],
                "transition": transition,
            }
        )


    # =====================================================
    # 1. Full point-by-point CSV
    # =====================================================

    csv_path = (
        OUT
        / f"state_transitions_cut_{cut}.csv"
    )

    with open(
        csv_path,
        "w",
        newline=""
    ) as f:

        writer = csv.DictWriter(
            f,
            fieldnames=records[0].keys()
        )

        writer.writeheader()
        writer.writerows(records)


    # =====================================================
    # 2. 3x3 core transition matrix
    # =====================================================

    matrix = np.zeros(
        (3, 3),
        dtype=int
    )

    for i, from_state in enumerate(CORE_STATES):

        for j, to_state in enumerate(CORE_STATES):

            matrix[i, j] = transition_counter[
                f"{from_state} -> {to_state}"
            ]


    matrix_csv = (
        OUT
        / f"transition_matrix_cut_{cut}.csv"
    )

    with open(
        matrix_csv,
        "w",
        newline=""
    ) as f:

        writer = csv.writer(f)

        writer.writerow(
            [
                "NoCut state",
                "Cut Fixed",
                "Cut Periodic",
                "Cut Chaotic",
            ]
        )

        for i, state in enumerate(CORE_STATES):

            writer.writerow(
                [
                    state,
                    matrix[i, 0],
                    matrix[i, 1],
                    matrix[i, 2],
                ]
            )


    # =====================================================
    # 3. Transition matrix PNG
    # =====================================================

    fig, ax = plt.subplots(
        figsize=(7, 6)
    )

    im = ax.imshow(
        matrix,
        cmap="Blues"
    )

    ax.set_xticks(
        range(3)
    )

    ax.set_yticks(
        range(3)
    )

    ax.set_xticklabels(
        CORE_STATES
    )

    ax.set_yticklabels(
        CORE_STATES
    )

    ax.set_xlabel(
        f"After Cut {cut}"
    )

    ax.set_ylabel(
        "No Cut"
    )

    ax.set_title(
        f"No Cut -> Cut {cut}: State Transitions"
    )

    for i in range(3):

        for j in range(3):

            value = matrix[i, j]

            ax.text(
                j,
                i,
                str(value),
                ha="center",
                va="center",
                fontsize=15
            )

    fig.colorbar(
        im,
        ax=ax,
        label="Number of parameter points"
    )

    fig.tight_layout()

    matrix_png = (
        UPLOAD
        / f"transition_matrix_cut_{cut}.png"
    )

    fig.savefig(
        matrix_png,
        dpi=180,
        bbox_inches="tight"
    )

    plt.close(fig)


    # =====================================================
    # 4. Parameter-space transition map
    # =====================================================

    fig, ax = plt.subplots(
        figsize=(14, 8)
    )

    for transition, style in TRANSITION_STYLE.items():

        subset = [
            r
            for r in records
            if r["transition"] == transition
        ]

        if not subset:
            continue

        color, marker = style

        ax.scatter(
            [r["alpha2"] for r in subset],
            [r["nu"] for r in subset],
            s=85,
            marker=marker,
            color=color,
            label=transition
        )

    ax.set_xlabel(
        r"$\alpha_2$"
    )

    ax.set_ylabel(
        r"$\nu$"
    )

    ax.set_title(
        f"No Cut -> Cut {cut}: State Transition Map"
    )

    ax.set_xlim(
        -10.5,
        10.5
    )

    ax.set_ylim(
        0.05,
        0.95
    )

    ax.set_xticks(
        range(-10, 11)
    )

    ax.set_yticks(
        [
            0.1, 0.2, 0.3,
            0.4, 0.5, 0.6,
            0.7, 0.8, 0.9
        ]
    )

    ax.grid(
        True,
        alpha=0.25
    )

    ax.legend(
        bbox_to_anchor=(1.02, 1),
        loc="upper left"
    )

    fig.tight_layout()

    map_png = (
        UPLOAD
        / f"transition_map_cut_{cut}.png"
    )

    fig.savefig(
        map_png,
        dpi=180,
        bbox_inches="tight"
    )

    plt.close(fig)


    # =====================================================
    # 5. Summary
    # =====================================================

    total = len(records)

    summary_txt = (
        OUT
        / f"transition_summary_cut_{cut}.txt"
    )

    with open(
        summary_txt,
        "w"
    ) as f:

        f.write(
            f"No Cut -> Cut {cut}\n"
        )

        f.write(
            "====================\n\n"
        )

        f.write(
            f"Comparable parameter points: {total}\n\n"
        )

        for from_state in CORE_STATES:

            for to_state in CORE_STATES:

                transition = (
                    f"{from_state} -> {to_state}"
                )

                count = transition_counter[
                    transition
                ]

                pct = (
                    100.0 * count / total
                    if total
                    else 0.0
                )

                f.write(
                    f"{transition}: "
                    f"{count} "
                    f"({pct:.1f}%)\n"
                )

                all_summary_rows.append(
                    {
                        "cut": cut,
                        "transition": transition,
                        "count": count,
                        "percent_all_points": pct,
                    }
                )

        f.write("\nOther / unresolved transitions:\n")

        if other_counter:

            for transition, count in sorted(
                other_counter.items()
            ):

                pct = (
                    100.0 * count / total
                )

                f.write(
                    f"{transition}: "
                    f"{count} "
                    f"({pct:.1f}%)\n"
                )

        else:

            f.write("None\n")


    print("")
    print(f"Cut {cut}")

    for from_state in CORE_STATES:

        for to_state in CORE_STATES:

            transition = (
                f"{from_state} -> {to_state}"
            )

            count = transition_counter[
                transition
            ]

            if count > 0:

                print(
                    f"  {transition}: {count}"
                )

    if other_counter:

        print(
            f"  Other: {sum(other_counter.values())}"
        )


# =========================================================
# 6. Combined summary CSV
# =========================================================

combined_csv = (
    OUT
    / "all_cut_transition_summary.csv"
)

with open(
    combined_csv,
    "w",
    newline=""
) as f:

    writer = csv.DictWriter(
        f,
        fieldnames=[
            "cut",
            "transition",
            "count",
            "percent_all_points",
        ]
    )

    writer.writeheader()
    writer.writerows(
        all_summary_rows
    )


print("")
print("DONE")
print(f"Results: {OUT}")
print(f"Overleaf PNGs: {UPLOAD}")
