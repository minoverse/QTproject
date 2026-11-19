#!/usr/bin/env python3

from pathlib import Path
import csv
import numpy as np
import matplotlib.pyplot as plt


ROOT = Path(__file__).resolve().parents[1]

INPUT = ROOT / "dynamic_analysis/results/cut_comparison_analysis"

OUT = ROOT / "dynamic_analysis/results/high_period_validation"
UPLOAD = OUT / "UPLOAD_TO_OVERLEAF"

OUT.mkdir(parents=True, exist_ok=True)
UPLOAD.mkdir(parents=True, exist_ok=True)


FILES = {
    "NoCut": INPUT / "classification_NoCut.csv",
    "Cut_A": INPUT / "classification_Cut_A.csv",
    "Cut_B": INPUT / "classification_Cut_B.csv",
    "Cut_C": INPUT / "classification_Cut_C.csv",
    "Cut_D": INPUT / "classification_Cut_D.csv",
    "Cut_E": INPUT / "classification_Cut_E.csv",
}


TARGET_PERIODS = {8, 10, 14, 16}

SKIP = 5000
TAIL = 1000
MAX_PERIOD = 16
TOL = 1e-4


def load_y1(path):
    x = np.loadtxt(
        path,
        usecols=(0,),
        skiprows=SKIP
    )

    x = np.asarray(x, dtype=float)
    x = x[np.isfinite(x)]

    return x


def period_error(y, p):
    if len(y) <= p:
        return np.inf

    scale = max(
        1.0,
        float(np.max(np.abs(y)))
    )

    diff = y[p:] - y[:-p]

    rmse = np.sqrt(
        np.mean(diff * diff)
    )

    return float(rmse / scale)


def find_min_period(y):
    errors = {}

    for p in range(1, MAX_PERIOD + 1):
        errors[p] = period_error(y, p)

    valid = [
        p for p in range(1, MAX_PERIOD + 1)
        if errors[p] <= TOL
    ]

    if valid:
        return min(valid), errors

    return None, errors


all_results = []


for dataset, csv_path in FILES.items():

    if not csv_path.exists():
        raise SystemExit(
            f"Missing file: {csv_path}"
        )

    with open(csv_path, newline="") as f:

        reader = csv.DictReader(f)

        for row in reader:

            if row["period"] == "":
                continue

            detected_period = int(
                float(row["period"])
            )

            if detected_period not in TARGET_PERIODS:
                continue

            alpha2 = float(row["alpha2"])
            nu = float(row["nu"])
            K = float(row["K"])

            result_file = Path(
                row["result_file"]
            )

            if not result_file.exists():

                print(
                    f"WARNING missing: {result_file}"
                )

                continue

            x = load_y1(result_file)

            if len(x) < TAIL:
                y = x
            else:
                y = x[-TAIL:]

            verified_period, errors = find_min_period(y)

            candidate_error = errors[
                detected_period
            ]

            status = (
                "CONFIRMED"
                if verified_period == detected_period
                else "REVISED"
            )

            if verified_period is None:
                status = "NOT_CONFIRMED"

            record = {
                "dataset": dataset,
                "alpha2": alpha2,
                "nu": nu,
                "K": K,
                "detected_period": detected_period,
                "verified_min_period": (
                    ""
                    if verified_period is None
                    else verified_period
                ),
                "candidate_nrmse": candidate_error,
                "status": status,
                "period_less_than_8": (
                    ""
                    if verified_period is None
                    else verified_period < 8
                ),
                "result_file": str(result_file),
            }

            all_results.append(record)

            case_name = (
                f"{dataset}_a2_{alpha2:g}_"
                f"nu_{nu:g}_p{detected_period}"
            )

            # -----------------------------------------
            # Plot 1: error for p = 1...16
            # -----------------------------------------

            periods = np.arange(
                1,
                MAX_PERIOD + 1
            )

            vals = np.array(
                [errors[p] for p in periods]
            )

            fig, ax = plt.subplots(
                figsize=(10, 5)
            )

            ax.plot(
                periods,
                vals,
                marker="o"
            )

            ax.axhline(
                TOL,
                linestyle="--",
                label=f"tolerance = {TOL:g}"
            )

            ax.set_yscale("log")

            ax.set_xticks(periods)

            ax.set_xlabel(
                "Candidate period p"
            )

            ax.set_ylabel(
                "Normalized RMSE"
            )

            ax.set_title(
                f"{dataset}: alpha2={alpha2:g}, "
                f"nu={nu:g}, detected p={detected_period}"
            )

            ax.grid(True, alpha=0.3)
            ax.legend()

            fig.tight_layout()

            fig.savefig(
                UPLOAD
                / f"{case_name}_period_error.png",
                dpi=180,
                bbox_inches="tight"
            )

            plt.close(fig)

            # -----------------------------------------
            # Plot 2: y[n] vs y[n+p]
            # -----------------------------------------

            p_show = (
                verified_period
                if verified_period is not None
                else detected_period
            )

            compare_len = min(
                200,
                len(y) - p_show
            )

            n = np.arange(compare_len)

            fig, ax = plt.subplots(
                figsize=(11, 5)
            )

            ax.plot(
                n,
                y[:compare_len],
                marker=".",
                label="y[n]"
            )

            ax.plot(
                n,
                y[p_show:p_show + compare_len],
                marker=".",
                label=f"y[n+{p_show}]"
            )

            ax.set_xlabel("n")
            ax.set_ylabel("y1")

            ax.set_title(
                f"{dataset}: period verification, "
                f"alpha2={alpha2:g}, nu={nu:g}"
            )

            ax.grid(True, alpha=0.3)
            ax.legend()

            fig.tight_layout()

            fig.savefig(
                UPLOAD
                / f"{case_name}_overlay.png",
                dpi=180,
                bbox_inches="tight"
            )

            plt.close(fig)

            # -----------------------------------------
            # Save all p errors
            # -----------------------------------------

            with open(
                OUT / f"{case_name}_errors.csv",
                "w",
                newline=""
            ) as ef:

                writer = csv.writer(ef)

                writer.writerow(
                    ["period", "normalized_RMSE"]
                )

                for p in periods:
                    writer.writerow(
                        [p, errors[p]]
                    )


# =========================================================
# Summary CSV
# =========================================================

summary_csv = OUT / "high_period_validation.csv"

with open(
    summary_csv,
    "w",
    newline=""
) as f:

    fieldnames = [
        "dataset",
        "alpha2",
        "nu",
        "K",
        "detected_period",
        "verified_min_period",
        "candidate_nrmse",
        "status",
        "period_less_than_8",
        "result_file",
    ]

    writer = csv.DictWriter(
        f,
        fieldnames=fieldnames
    )

    writer.writeheader()
    writer.writerows(all_results)


# =========================================================
# Text summary
# =========================================================

summary_txt = OUT / "high_period_validation.txt"

with open(summary_txt, "w") as f:

    f.write(
        "High-period validation\n"
        "======================\n\n"
    )

    f.write(
        f"Transient skip = {SKIP}\n"
        f"Tail samples = {TAIL}\n"
        f"Period tolerance = {TOL}\n"
        f"Periods tested = 1..{MAX_PERIOD}\n\n"
    )

    for r in all_results:

        f.write(
            f"{r['dataset']} | "
            f"alpha2={r['alpha2']:g} | "
            f"nu={r['nu']:g} | "
            f"K={r['K']:.6f} | "
            f"original p={r['detected_period']} | "
            f"minimum verified p={r['verified_min_period']} | "
            f"status={r['status']} | "
            f"p<8={r['period_less_than_8']}\n"
        )

    confirmed_ge8 = [
        r for r in all_results
        if (
            r["verified_min_period"] != ""
            and int(r["verified_min_period"]) >= 8
        )
    ]

    confirmed_lt8 = [
        r for r in all_results
        if (
            r["verified_min_period"] != ""
            and int(r["verified_min_period"]) < 8
        )
    ]

    f.write("\n")
    f.write(
        f"Verified minimum period < 8: "
        f"{len(confirmed_lt8)}\n"
    )

    f.write(
        f"Verified minimum period >= 8: "
        f"{len(confirmed_ge8)}\n"
    )

    if confirmed_ge8:
        f.write(
            "\nConclusion: some verified periodic cases "
            "have period >= 8, so a universal "
            "'period < 8' assumption is NOT supported.\n"
        )
    else:
        f.write(
            "\nConclusion: all tested high-period candidates "
            "reduce to minimum period < 8 under this "
            "verification criterion.\n"
        )


print("")
print("DONE")
print(f"Summary: {summary_txt}")
print(f"CSV: {summary_csv}")
print(f"Plots: {UPLOAD}")
