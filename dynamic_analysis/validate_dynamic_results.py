#!/usr/bin/env python3

from pathlib import Path
import csv
import re

HERE = Path(__file__).resolve().parent
RESULTS = HERE / "results"
CODE = (HERE / "dynamic_analysis.py").read_text()

EDGES = {
    "A": ("s14", "s41"),
    "B": ("s25", "s52"),
    "C": ("s13", "s31"),
    "D": ("s23", "s32"),
    "E": ("s12", "s21"),
}

ok = True

def check(name, cond):
    global ok
    print(("PASS" if cond else "FAIL"), "-", name)
    ok &= cond

print("\n=== CODE / DYNAMIC INPUT ===")

check(
    "Bifurcation x-axis selectable: alpha2 / nu",
    "Bif x-axis alpha2 or nu" in CODE
)

check(
    "Bifurcation min/max/step user input",
    all(x in CODE for x in ["Bif min", "Bif max", "Bif step"])
)

check(
    "Bifurcation dot size user input",
    "Bif dot size" in CODE and "--bif-point-size" in CODE
)

check(
    "K-map dot size user input",
    "K-map dot size" in CODE and "--k-point-size" in CODE
)

check(
    "AUTO A..E",
    "AUTO A..E" in CODE
)

for edge, (a, b) in EDGES.items():
    check(
        f"Cut {edge} = {a},{b}",
        f'"{edge}": ["{a}", "{b}"]' in CODE
    )


print("\n=== GENERATED RESULTS ===")

for edge in EDGES:
    bif_dirs = sorted(
        RESULTS.glob(f"bif_cut_{edge}_*"),
        key=lambda p: p.stat().st_mtime,
        reverse=True
    )

    k_dirs = sorted(
        RESULTS.glob(f"kmap_cut_{edge}_*"),
        key=lambda p: p.stat().st_mtime,
        reverse=True
    )

    bif = next(
        (
            d for d in bif_dirs
            if len(list((d / "UPLOAD_TO_OVERLEAF").glob("*.png"))) >= 5
        ),
        None
    )

    km = next(
        (
            d for d in k_dirs
            if list((d / "UPLOAD_TO_OVERLEAF").glob("*.png"))
            and list((d / "UPLOAD_TO_OVERLEAF").glob("*.csv"))
        ),
        None
    )

    check(f"Cut {edge} bifurcation complete", bif is not None)
    check(f"Cut {edge} K-map complete", km is not None)

    if bif:
        dat = bif / "bifurcation.dat"
        xs = set()

        if dat.exists():
            for line in dat.read_text().splitlines():
                cols = line.split()
                if cols:
                    xs.add(float(cols[0]))

        gp = bif / "bif_y1.gnu"
        axis = "unknown"
        point = "unknown"

        if gp.exists():
            text = gp.read_text()
            m = re.search(r"set xlabel '([^']+)'", text)
            if m:
                axis = m.group(1)
            m = re.search(r"\bps ([0-9.]+)", text)
            if m:
                point = m.group(1)

        print(
            f"     BIF {edge}: x-axis={axis}, "
            f"x-values={len(xs)}, "
            f"range={min(xs) if xs else '?'}..{max(xs) if xs else '?'}, "
            f"dot={point}"
        )

    if km:
        csvs = list(
            (km / "UPLOAD_TO_OVERLEAF").glob("K_results*.csv")
        )

        if csvs:
            with csvs[0].open() as f:
                rows = list(csv.DictReader(f))

            skipped = sum(
                r["status"] == "SKIPPED_NU_ZERO"
                for r in rows
            )

            valid = sum(
                bool(r["K"])
                for r in rows
            )

            gp = km / "K_map.gnu"
            point = "unknown"

            if gp.exists():
                m = re.search(r"\bps ([0-9.]+)", gp.read_text())
                if m:
                    point = m.group(1)

            print(
                f"     KMAP {edge}: rows={len(rows)}, "
                f"K-values={valid}, "
                f"nu=0 skipped={skipped}, "
                f"dot={point}"
            )


print("\n========================")
print("FINAL:", "ALL PASS" if ok else "CHECK FAILURES")
print("========================")

raise SystemExit(0 if ok else 1)
