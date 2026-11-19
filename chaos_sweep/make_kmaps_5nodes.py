#!/usr/bin/env python3
from __future__ import annotations

import csv
import math
import os
import re
import subprocess
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

HERE = Path(__file__).resolve().parent
BUILD = HERE / "build"
TEST_BIN = BUILD / "test01_multi"

RESULT = HERE / "results" / "chaos_grid_a2_-10_10_nu_-0.1_0.9"
SERIES = RESULT / "series"
UPLOAD = RESULT / "UPLOAD_TO_OVERLEAF"

WORKERS = int(os.environ.get("BN_WORKERS", os.cpu_count() or 1))
SKIP = int(os.environ.get("CHAOS_SKIP", "5000"))

ALPHAS = list(range(-10, 11))
NUS = [round(-0.1 + 0.1 * i, 1) for i in range(11)]

K_RE = re.compile(r"K \(median\)\s*:\s*(\S+)")

def tag(x: float) -> str:
    if abs(x - round(x)) < 1e-12:
        return str(int(round(x)))
    return f"{x:.1f}"

def case_dir(alpha: int, nu: float) -> Path:
    return SERIES / f"a2_{tag(alpha)}_nu_{tag(nu)}"

def one_case(alpha: int, nu: float, col: int):
    d = case_dir(alpha, nu)
    inp = d / "result.dat"

    if abs(nu) < 1e-14:
        return (alpha, nu, None, "SKIPPED_NU_ZERO")

    if not inp.exists():
        return (alpha, nu, None, "NO_RESULT")

    p = subprocess.run(
        [str(TEST_BIN), str(SKIP), str(col), "result.dat"],
        cwd=d,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )

    if p.returncode != 0:
        return (alpha, nu, None, "TEST_ERROR")

    m = K_RE.search(p.stdout)
    if not m:
        return (alpha, nu, None, "PARSE_ERROR")

    try:
        k = float(m.group(1))
    except ValueError:
        return (alpha, nu, None, "INVALID_K")

    if not math.isfinite(k):
        return (alpha, nu, None, "INVALID_K")

    return (alpha, nu, k, "OK")

def write_gnuplot_and_run(script: Path, cwd: Path):
    p = subprocess.run(["gnuplot", script.name], cwd=cwd, text=True,
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if p.returncode != 0:
        raise RuntimeError(p.stderr.strip() or "gnuplot failed")

def main():
    if not TEST_BIN.exists():
        raise SystemExit("Build test01_multi first.")

    UPLOAD.mkdir(parents=True, exist_ok=True)

    jobs = [(a, n) for a in ALPHAS for n in NUS]

    for col in range(5):
        node = col + 1
        print(f"\n=== y{node} ===", flush=True)

        rows = []
        with ThreadPoolExecutor(max_workers=WORKERS) as ex:
            futs = [ex.submit(one_case, a, n, col) for a, n in jobs]
            for i, fut in enumerate(as_completed(futs), 1):
                r = fut.result()
                rows.append(r)
                msg = f"[y{node} {i}/{len(futs)}] a2={r[0]} nu={r[1]:.1f} {r[3]}"
                if r[2] is not None:
                    msg += f" K={r[2]:.6f}"
                print(msg, flush=True)

        rows.sort(key=lambda x: (x[1], x[0]))

        csv_path = RESULT / f"K_results_y{node}.csv"
        with csv_path.open("w", newline="") as f:
            w = csv.writer(f)
            w.writerow(["alpha2", "nu", "K", "status"])
            for row in rows:
                w.writerow(row)

        dat_path = RESULT / f"K_map_y{node}.dat"
        with dat_path.open("w") as f:
            for alpha, nu, k, status in rows:
                if status == "OK" and k is not None:
                    f.write(f"{alpha} {nu:.1f} {k:.12g}\n")

        gp_path = RESULT / f"K_map_y{node}.gnu"
        gp_path.write_text(
            "set terminal pngcairo size 1500,950 enhanced\n"
            f"set output 'K_map_y{node}.png'\n"
            f"set title '0-1 Chaos Test: K over alpha2 and nu (y{node})'\n"
            "set xlabel 'alpha2'\n"
            "set ylabel 'nu'\n"
            "set cblabel 'K'\n"
            "set cbrange [0:1]\n"
            "set xrange [-10.5:10.5]\n"
            "set yrange [-0.15:0.95]\n"
            "set grid\n"
            f"plot 'K_map_y{node}.dat' using 1:2:3 with points pt 7 ps 2 palette notitle\n"
        )

        write_gnuplot_and_run(gp_path, RESULT)

        src_png = RESULT / f"K_map_y{node}.png"
        dst_png = UPLOAD / f"step2_K_map_y{node}.png"
        dst_png.write_bytes(src_png.read_bytes())

        print(f"Saved: {dst_png}", flush=True)

    print("\nDONE")
    print(f"Upload folder: {UPLOAD}")

if __name__ == "__main__":
    main()
