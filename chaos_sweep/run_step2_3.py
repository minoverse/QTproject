#!/usr/bin/env python3
from __future__ import annotations

import csv
import math
import os
import re
import shutil
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

HERE = Path(__file__).resolve().parent
BUILD = HERE / "build"
CASE_BIN = BUILD / "chaos_case"
TEST_BIN = BUILD / "test01"
RESULT = HERE / "results" / "chaos_grid_a2_-10_10_nu_-0.1_0.9"
SERIES = RESULT / "series"
UPLOAD = RESULT / "UPLOAD_TO_OVERLEAF"

WORKERS = int(os.environ.get("BN_WORKERS", os.cpu_count() or 1))
SKIP = int(os.environ.get("CHAOS_SKIP", "5000"))

ALPHAS = list(range(-10, 11))
NUS = [round(-0.1 + 0.1 * i, 1) for i in range(11)]

K_RE = re.compile(r"K \(median\)\s*:\s*([-+0-9.eE]+)")
KMM_RE = re.compile(r"K min / max\s*:\s*([-+0-9.eE]+)\s*/\s*([-+0-9.eE]+)")


def tag(x: float) -> str:
    if abs(x - round(x)) < 1e-12:
        return str(int(round(x)))
    return f"{x:.1f}"


def case_dir(alpha: int, nu: float) -> Path:
    return SERIES / f"a2_{tag(alpha)}_nu_{tag(nu)}"


def generate_one(alpha: int, nu: float):
    d = case_dir(alpha, nu)
    d.mkdir(parents=True, exist_ok=True)
    out = d / "result.dat"
    if out.exists() and out.stat().st_size > 1000:
        return alpha, nu, "EXISTING", ""
    if abs(nu) < 1e-14:
        return alpha, nu, "SKIPPED_NU_ZERO", ""
    p = subprocess.run(
        [str(CASE_BIN), str(alpha), f"{nu:.1f}", str(out)],
        cwd=d,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    if p.returncode != 0:
        return alpha, nu, "SIM_ERROR", (p.stderr or p.stdout).strip()
    return alpha, nu, "OK", ""


def test_one(alpha: int, nu: float):
    d = case_dir(alpha, nu)
    inp = d / "result.dat"
    if not inp.exists():
        return alpha, nu, None, None, None, "NO_SERIES"
    p = subprocess.run(
        [str(TEST_BIN), str(SKIP), "result.dat"],
        cwd=d,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    (d / "test01.out").write_text(p.stdout + ("\nSTDERR:\n" + p.stderr if p.stderr else ""))
    if p.returncode != 0:
        return alpha, nu, None, None, None, "TEST_ERROR"
    m = K_RE.search(p.stdout)
    mm = KMM_RE.search(p.stdout)
    if not m or not mm:
        return alpha, nu, None, None, None, "PARSE_ERROR"
    return alpha, nu, float(m.group(1)), float(mm.group(1)), float(mm.group(2)), "OK"


def write_gnuplot_and_run(script: Path, cwd: Path):
    p = subprocess.run(["gnuplot", script.name], cwd=cwd, text=True,
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if p.returncode != 0:
        raise RuntimeError(p.stderr.strip() or "gnuplot failed")


def make_k_map(rows):
    UPLOAD.mkdir(parents=True, exist_ok=True)
    dat = RESULT / "K_map.dat"
    with dat.open("w") as f:
        for alpha, nu, k, kmin, kmax, status in rows:
            if status == "OK":
                f.write(f"{alpha} {nu:.1f} {k:.12g}\n")
    gp = RESULT / "K_map.gnu"
    gp.write_text(
        "set terminal pngcairo size 1500,950 enhanced\n"
        "set output 'K_map.png'\n"
        "set title '0-1 Chaos Test: K over alpha2 and nu (y1)'\n"
        "set xlabel 'alpha2'\n"
        "set ylabel 'nu'\n"
        "set cblabel 'K'\n"
        "set cbrange [0:1]\n"
        "set xrange [-10.5:10.5]\n"
        "set yrange [-0.15:0.95]\n"
        "set grid\n"
        "plot 'K_map.dat' using 1:2:3 with points pt 7 ps 2 palette notitle\n"
    )
    write_gnuplot_and_run(gp, RESULT)
    shutil.copy2(RESULT / "K_map.png", UPLOAD / "step2_K_map.png")


def run_pq(alpha: int, nu: float, label: str, k: float):
    d = case_dir(alpha, nu)
    p = subprocess.run(
        [str(TEST_BIN), str(SKIP), "pq", "result.dat"],
        cwd=d,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    if p.returncode != 0 or not (d / "pq.dat").exists():
        raise RuntimeError(f"P-Q failed for {label}: {p.stderr}")
    gp = d / f"pq_{label}.gnu"
    gp.write_text(
        "set terminal pngcairo size 1000,1000 enhanced\n"
        f"set output 'pq_{label}.png'\n"
        f"set title '{label.capitalize()} example: alpha2={alpha}, nu={nu:.1f}, K={k:.4f}'\n"
        "set xlabel 'p'\n"
        "set ylabel 'q'\n"
        "set size square\n"
        "set grid\n"
        "unset key\n"
        "plot 'pq.dat' using 2:3 with lines lw 1\n"
    )
    write_gnuplot_and_run(gp, d)
    dst = UPLOAD / f"step3_pq_{label}.png"
    shutil.copy2(d / f"pq_{label}.png", dst)
    return dst


def make_tex(rows, regular, chaotic):
    tex = UPLOAD / "plots_step2_step3.tex"
    a0, n0, k0 = regular
    a1, n1, k1 = chaotic
    tex.write_text(r"""% Add \usepackage{graphicx} in the preamble.
\section{0--1 Chaos Test}
\begin{figure}[htbp]
\centering
\includegraphics[width=0.82\linewidth]{step2_K_map.png}
\caption{Median $K$ from the 0--1 chaos test for $y_1$ over $(\alpha_2,\nu)$. Values near 0 indicate regular dynamics; values near 1 indicate chaotic dynamics.}
\end{figure}

\section{P--Q Examples}
\begin{figure}[htbp]
\centering
\begin{minipage}{0.48\linewidth}
\centering
\includegraphics[width=\linewidth]{step3_pq_regular.png}
\caption*{Regular example: bounded $p$--$q$ trajectory.}
\end{minipage}
\hfill
\begin{minipage}{0.48\linewidth}
\centering
\includegraphics[width=\linewidth]{step3_pq_chaotic.png}
\caption*{Chaotic example: diffusive/non-bounded $p$--$q$ trajectory.}
\end{minipage}
\end{figure}
""")
    (UPLOAD / "selected_cases.txt").write_text(
        f"regular: alpha2={a0}, nu={n0:.1f}, K={k0:.8f}\n"
        f"chaotic: alpha2={a1}, nu={n1:.1f}, K={k1:.8f}\n"
        f"CHAOS_SKIP={SKIP}\n"
        "nu=0 was skipped because the unchanged direct GAMMA evaluation is singular there.\n"
    )


def main():
    if not CASE_BIN.exists() or not TEST_BIN.exists():
        print("Build first with ./build_step2_3.sh", file=sys.stderr)
        return 2
    SERIES.mkdir(parents=True, exist_ok=True)
    UPLOAD.mkdir(parents=True, exist_ok=True)

    jobs = [(a, n) for a in ALPHAS for n in NUS]
    print(f"Workers={WORKERS}; simulation cases={len(jobs)}; professor test skip={SKIP}", flush=True)
    print("nu=0 cases will be recorded as skipped; numerical core is not modified.", flush=True)

    sim_status = []
    with ThreadPoolExecutor(max_workers=WORKERS) as ex:
        futures = [ex.submit(generate_one, a, n) for a, n in jobs]
        for i, fut in enumerate(as_completed(futures), 1):
            r = fut.result()
            sim_status.append(r)
            print(f"[SIM {i}/{len(futures)}] a2={r[0]} nu={r[1]:.1f} {r[2]}", flush=True)

    test_jobs = [(a, n) for a, n, st, msg in sim_status if st in ("OK", "EXISTING")]
    rows = []
    with ThreadPoolExecutor(max_workers=WORKERS) as ex:
        futures = [ex.submit(test_one, a, n) for a, n in test_jobs]
        for i, fut in enumerate(as_completed(futures), 1):
            r = fut.result()
            rows.append(r)
            print(f"[K {i}/{len(futures)}] a2={r[0]} nu={r[1]:.1f} {r[5]}"
                  + (f" K={r[2]:.6f}" if r[2] is not None else ""), flush=True)

    for a, n, st, msg in sim_status:
        if st == "SKIPPED_NU_ZERO":
            rows.append((a, n, None, None, None, st))
        elif st not in ("OK", "EXISTING"):
            rows.append((a, n, None, None, None, st))

    rows.sort(key=lambda r: (r[1], r[0]))
    csv_path = RESULT / "K_results.csv"
    with csv_path.open("w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["alpha2", "nu", "K", "K_min", "K_max", "status"])
        for row in rows:
            w.writerow(row)

    valid = [(a, n, k) for a, n, k, _, _, st in rows if st == "OK" and k is not None and math.isfinite(k)]
    if not valid:
        raise RuntimeError("No valid K results.")

    regular = min(valid, key=lambda x: abs(x[2] - 0.0))
    chaotic = min(valid, key=lambda x: abs(x[2] - 1.0))
    make_k_map(rows)
    run_pq(regular[0], regular[1], "regular", regular[2])
    run_pq(chaotic[0], chaotic[1], "chaotic", chaotic[2])
    make_tex(rows, regular, chaotic)

    print("\nDONE")
    print(f"K CSV: {csv_path}")
    print(f"Overleaf files: {UPLOAD}")
    print(f"Regular example: a2={regular[0]}, nu={regular[1]:.1f}, K={regular[2]:.6f}")
    print(f"Chaotic example: a2={chaotic[0]}, nu={chaotic[1]:.1f}, K={chaotic[2]:.6f}")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
