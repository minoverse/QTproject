#!/usr/bin/env python3

import argparse
import csv
import hashlib
import json
import math
import os
import re
import shutil
import subprocess

from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path


HERE = Path(__file__).resolve().parent
BUILD = HERE / "build"
CASE_BIN = BUILD / "dynamic_case"
TEST_BIN = BUILD / "test01_dynamic"
RESULTS = HERE / "results"

CONNECTIONS = [
    "s12", "s13", "s14",
    "s21", "s23", "s25",
    "s31", "s32", "s33",
    "s41", "s52"
]

K_RE = re.compile(r"K \(median\)\s*:\s*([-+0-9.eE]+)")
KMM_RE = re.compile(
    r"K min / max\s*:\s*([-+0-9.eE]+)\s*/\s*([-+0-9.eE]+)"
)


def fmt(x):
    if abs(x - round(x)) < 1e-12:
        return str(int(round(x)))

    return f"{x:.8g}"


def frange(start, stop, step):
    if step == 0:
        raise ValueError("step cannot be zero")

    values = []
    i = 0

    while True:
        value = start + i * step

        if step > 0 and value > stop + 1e-10:
            break

        if step < 0 and value < stop - 1e-10:
            break

        values.append(round(value, 12))
        i += 1

    return values


def node_cuts(node):
    s = str(node)

    return [
        c for c in CONNECTIONS
        if c[1] == s or c[2] == s
    ]


def parse_sij(text):
    cuts = []

    for item in text.split(","):
        item = item.strip().lower()

        if not item:
            continue

        if item not in CONNECTIONS:
            raise ValueError(
                "Unknown connection: " + item
            )

        if item not in cuts:
            cuts.append(item)

    return cuts


def cut_sets(args):
    if args.cut_mode == "node":
        return [
            (
                f"node{args.node}",
                node_cuts(args.node)
            )
        ]

    if args.cut_mode == "sij":
        cuts = parse_sij(args.sij)

        return [
            (
                "sij_" + "_".join(cuts),
                cuts
            )
        ]

    return [
        (
            f"node{node}",
            node_cuts(node)
        )
        for node in range(1, 6)
    ]


def config_id(data):
    raw = json.dumps(
        data,
        sort_keys=True
    ).encode()

    return hashlib.sha1(
        raw
    ).hexdigest()[:10]


def run_case(
    alpha2,
    nu,
    tmax,
    output,
    done,
    cuts
):
    metadata = {
        "alpha2": alpha2,
        "nu": nu,
        "tmax": tmax,
        "cuts": cuts
    }

    if done.exists() and output.exists():
        try:
            old = json.loads(
                done.read_text()
            )

            if (
                old == metadata
                and output.stat().st_size > 0
            ):
                return "EXISTING", ""

        except Exception:
            pass

    if abs(nu) < 1e-14:
        return "SKIPPED_NU_ZERO", ""

    output.parent.mkdir(
        parents=True,
        exist_ok=True
    )

    p = subprocess.run(
        [
            str(CASE_BIN),
            fmt(alpha2),
            fmt(nu),
            str(tmax),
            str(output),
            ",".join(cuts)
        ],
        cwd=output.parent,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )

    if p.returncode != 0:
        return (
            "SIM_ERROR",
            p.stderr.strip()
        )

    done.write_text(
        json.dumps(
            metadata,
            indent=2,
            sort_keys=True
        )
    )

    return "OK", ""


def gnuplot(script, cwd):
    p = subprocess.run(
        [
            "gnuplot",
            script.name
        ],
        cwd=cwd,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )

    if p.returncode != 0:
        raise RuntimeError(
            p.stderr
        )


def run_bif(args, label, cuts):
    x_values = frange(
        args.bif_min,
        args.bif_max,
        args.bif_step
    )

    config = {
        "label": label,
        "cuts": cuts,
        "x": args.bif_x,
        "min": args.bif_min,
        "max": args.bif_max,
        "step": args.bif_step,
        "alpha2": args.alpha2,
        "nu": args.nu,
        "tmax": args.bif_tmax,
        "transient": args.transient
    }

    run_dir = RESULTS / (
        "bif_"
        + label
        + "_"
        + config_id(config)
    )

    series = run_dir / "series"
    upload = run_dir / "UPLOAD_TO_OVERLEAF"

    series.mkdir(
        parents=True,
        exist_ok=True
    )

    upload.mkdir(
        parents=True,
        exist_ok=True
    )

    jobs = []

    for x in x_values:

        if args.bif_x == "alpha2":
            alpha2 = x
            nu = args.nu

        else:
            alpha2 = args.alpha2
            nu = x

        case = series / (
            "x_" + fmt(x)
        )

        jobs.append(
            (
                x,
                alpha2,
                nu,
                case / "result.dat",
                case / "done.json"
            )
        )

    status = {}

    print()
    print(
        f"[BIF {label}] "
        f"{len(jobs)} cases"
    )

    with ThreadPoolExecutor(
        max_workers=args.workers
    ) as executor:

        futures = {}

        for (
            x,
            alpha2,
            nu,
            output,
            done
        ) in jobs:

            future = executor.submit(
                run_case,
                alpha2,
                nu,
                args.bif_tmax,
                output,
                done,
                cuts
            )

            futures[future] = (
                x,
                alpha2,
                nu
            )

        for i, future in enumerate(
            as_completed(futures),
            1
        ):
            x, alpha2, nu = futures[future]

            st, msg = future.result()

            status[x] = st

            print(
                f"[BIF {i}/{len(futures)}] "
                f"x={fmt(x)} "
                f"{st}",
                flush=True
            )

            if msg:
                print(msg)


    dat = run_dir / "bifurcation.dat"

    with dat.open("w") as f:

        for (
            x,
            alpha2,
            nu,
            output,
            done
        ) in jobs:

            if status.get(x) not in (
                "OK",
                "EXISTING"
            ):
                continue

            with output.open() as source:

                for t, line in enumerate(
                    source,
                    1
                ):

                    if t < args.transient:
                        continue

                    values = line.split()

                    if len(values) != 5:
                        continue

                    f.write(
                        fmt(x)
                        + " "
                        + " ".join(values)
                        + "\n"
                    )

            f.write("\n")


    for node in range(1, 6):

        png = (
            f"bif_{label}_"
            f"{args.bif_x}_"
            f"y{node}.png"
        )

        gp = run_dir / (
            f"bif_y{node}.gnu"
        )

        gp.write_text(
            "set terminal pngcairo "
            "size 1500,950 enhanced\n"
            f"set output '{png}'\n"
            f"set title "
            f"'Bifurcation y{node}: "
            f"{label}'\n"
            f"set xlabel "
            f"'{args.bif_x}'\n"
            f"set ylabel 'y{node}'\n"
            "set grid\n"
            "unset key\n"
            f"plot 'bifurcation.dat' "
            f"using 1:{node + 1} "
            f"with points pt 7 "
            f"ps {args.bif_point_size} "
            f"notitle\n"
        )

        gnuplot(
            gp,
            run_dir
        )

        shutil.copy2(
            run_dir / png,
            upload / png
        )

    print(
        "BIF DONE:",
        upload
    )


def run_k_test(case, skip):
    cache = (
        case
        / f"K_skip_{skip}.json"
    )

    if cache.exists():
        try:
            data = json.loads(
                cache.read_text()
            )

            return (
                data["K"],
                data["K_min"],
                data["K_max"],
                "EXISTING"
            )

        except Exception:
            pass

    p = subprocess.run(
        [
            str(TEST_BIN),
            str(skip),
            "result.dat"
        ],
        cwd=case,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )

    if p.returncode != 0:
        return (
            None,
            None,
            None,
            "TEST_ERROR"
        )

    match = K_RE.search(
        p.stdout
    )

    mm = KMM_RE.search(
        p.stdout
    )

    if not match or not mm:
        return (
            None,
            None,
            None,
            "PARSE_ERROR"
        )

    data = {
        "K": float(
            match.group(1)
        ),
        "K_min": float(
            mm.group(1)
        ),
        "K_max": float(
            mm.group(2)
        )
    }

    cache.write_text(
        json.dumps(
            data,
            indent=2
        )
    )

    return (
        data["K"],
        data["K_min"],
        data["K_max"],
        "OK"
    )


def run_kmap(args, label, cuts):
    alphas = frange(
        args.alpha_min,
        args.alpha_max,
        args.alpha_step
    )

    nus = frange(
        args.nu_min,
        args.nu_max,
        args.nu_step
    )

    config = {
        "label": label,
        "cuts": cuts,
        "alpha_min": args.alpha_min,
        "alpha_max": args.alpha_max,
        "alpha_step": args.alpha_step,
        "nu_min": args.nu_min,
        "nu_max": args.nu_max,
        "nu_step": args.nu_step,
        "skip": args.skip
    }

    run_dir = RESULTS / (
        "kmap_"
        + label
        + "_"
        + config_id(config)
    )

    series = run_dir / "series"
    upload = run_dir / "UPLOAD_TO_OVERLEAF"

    series.mkdir(
        parents=True,
        exist_ok=True
    )

    upload.mkdir(
        parents=True,
        exist_ok=True
    )

    jobs = []

    for alpha2 in alphas:

        for nu in nus:

            case = series / (
                "a2_"
                + fmt(alpha2)
                + "_nu_"
                + fmt(nu)
            )

            jobs.append(
                (
                    alpha2,
                    nu,
                    case
                )
            )

    print()
    print(
        f"[KMAP {label}] "
        f"{len(jobs)} cases"
    )

    sim_status = {}

    with ThreadPoolExecutor(
        max_workers=args.workers
    ) as executor:

        futures = {}

        for (
            alpha2,
            nu,
            case
        ) in jobs:

            future = executor.submit(
                run_case,
                alpha2,
                nu,
                10000,
                case / "result.dat",
                case / "done.json",
                cuts
            )

            futures[future] = (
                alpha2,
                nu
            )

        for i, future in enumerate(
            as_completed(futures),
            1
        ):

            alpha2, nu = futures[future]

            st, msg = future.result()

            sim_status[
                (alpha2, nu)
            ] = st

            print(
                f"[SIM {i}/{len(futures)}] "
                f"a2={fmt(alpha2)} "
                f"nu={fmt(nu)} "
                f"{st}",
                flush=True
            )

            if msg:
                print(msg)


    test_jobs = []

    for (
        alpha2,
        nu,
        case
    ) in jobs:

        if sim_status.get(
            (alpha2, nu)
        ) in (
            "OK",
            "EXISTING"
        ):

            test_jobs.append(
                (
                    alpha2,
                    nu,
                    case
                )
            )


    rows = []

    with ThreadPoolExecutor(
        max_workers=args.workers
    ) as executor:

        futures = {}

        for (
            alpha2,
            nu,
            case
        ) in test_jobs:

            future = executor.submit(
                run_k_test,
                case,
                args.skip
            )

            futures[future] = (
                alpha2,
                nu
            )

        for i, future in enumerate(
            as_completed(futures),
            1
        ):

            alpha2, nu = futures[future]

            (
                K,
                Kmin,
                Kmax,
                st
            ) = future.result()

            rows.append(
                (
                    alpha2,
                    nu,
                    K,
                    Kmin,
                    Kmax,
                    st
                )
            )

            if K is None:

                print(
                    f"[K {i}/{len(futures)}] "
                    f"{st}"
                )

            else:

                print(
                    f"[K {i}/{len(futures)}] "
                    f"a2={fmt(alpha2)} "
                    f"nu={fmt(nu)} "
                    f"K={K:.6f}"
                )


    for (
        alpha2,
        nu,
        case
    ) in jobs:

        st = sim_status.get(
            (alpha2, nu)
        )

        if st not in (
            "OK",
            "EXISTING"
        ):

            rows.append(
                (
                    alpha2,
                    nu,
                    None,
                    None,
                    None,
                    st
                )
            )


    rows.sort(
        key=lambda row: (
            row[1],
            row[0]
        )
    )


    csv_file = (
        run_dir
        / f"K_results_skip_{args.skip}.csv"
    )

    with csv_file.open(
        "w",
        newline=""
    ) as f:

        writer = csv.writer(f)

        writer.writerow(
            [
                "alpha2",
                "nu",
                "K",
                "K_min",
                "K_max",
                "status"
            ]
        )

        writer.writerows(rows)


    dat = (
        run_dir
        / f"K_map_skip_{args.skip}.dat"
    )

    with dat.open("w") as f:

        for row in rows:

            (
                alpha2,
                nu,
                K,
                Kmin,
                Kmax,
                st
            ) = row

            if K is None:
                continue

            if not math.isfinite(K):
                continue

            f.write(
                f"{alpha2} "
                f"{nu} "
                f"{K}\n"
            )


    png = (
        f"kmap_{label}_"
        f"skip_{args.skip}.png"
    )

    gp = run_dir / "K_map.gnu"

    gp.write_text(
        "set terminal pngcairo "
        "size 1500,950 enhanced\n"
        f"set output '{png}'\n"
        f"set title "
        f"'0-1 Chaos Test: "
        f"{label}'\n"
        "set xlabel 'alpha2'\n"
        "set ylabel 'nu'\n"
        "set cblabel 'K'\n"
        "set cbrange [0:1]\n"
        "set grid\n"
        f"plot "
        f"'K_map_skip_{args.skip}.dat' "
        f"using 1:2:3 "
        f"with points pt 7 "
        f"ps {args.k_point_size} "
        f"palette notitle\n"
    )

    gnuplot(
        gp,
        run_dir
    )

    shutil.copy2(
        run_dir / png,
        upload / png
    )

    shutil.copy2(
        csv_file,
        upload / csv_file.name
    )

    print(
        "KMAP DONE:",
        upload
    )


def add_common(parser):
    parser.add_argument(
        "--cut-mode",
        choices=[
            "node",
            "sij",
            "auto"
        ],
        required=True
    )

    parser.add_argument(
        "--node",
        type=int,
        choices=[
            1,
            2,
            3,
            4,
            5
        ]
    )

    parser.add_argument(
        "--sij"
    )

    parser.add_argument(
        "--workers",
        type=int,
        default=8
    )


def add_bif(parser):
    parser.add_argument(
        "--bif-x",
        choices=[
            "alpha2",
            "nu"
        ],
        default="nu"
    )

    parser.add_argument(
        "--bif-min",
        type=float,
        default=0.1
    )

    parser.add_argument(
        "--bif-max",
        type=float,
        default=0.9
    )

    parser.add_argument(
        "--bif-step",
        type=float,
        default=0.05
    )

    parser.add_argument(
        "--alpha2",
        type=float,
        default=-10
    )

    parser.add_argument(
        "--nu",
        type=float,
        default=0.7
    )

    parser.add_argument(
        "--bif-tmax",
        type=int,
        default=10000
    )

    parser.add_argument(
        "--transient",
        type=int,
        default=5000
    )

    parser.add_argument(
        "--bif-point-size",
        type=float,
        default=0.25
    )


def add_kmap(parser):
    parser.add_argument(
        "--alpha-min",
        type=float,
        default=-10
    )

    parser.add_argument(
        "--alpha-max",
        type=float,
        default=10
    )

    parser.add_argument(
        "--alpha-step",
        type=float,
        default=1
    )

    parser.add_argument(
        "--nu-min",
        type=float,
        default=-0.1
    )

    parser.add_argument(
        "--nu-max",
        type=float,
        default=0.9
    )

    parser.add_argument(
        "--nu-step",
        type=float,
        default=0.1
    )

    parser.add_argument(
        "--skip",
        type=int,
        default=5000
    )

    parser.add_argument(
        "--k-point-size",
        type=float,
        default=2
    )


def make_parser():
    parser = argparse.ArgumentParser()

    sub = parser.add_subparsers(
        dest="command",
        required=True
    )

    bif = sub.add_parser("bif")
    add_common(bif)
    add_bif(bif)

    kmap = sub.add_parser("kmap")
    add_common(kmap)
    add_kmap(kmap)

    both = sub.add_parser("all")
    add_common(both)
    add_bif(both)
    add_kmap(both)

    sub.add_parser(
        "interactive"
    )

    return parser


def ask(text, default):
    value = input(
        f"{text} [{default}]: "
    ).strip()

    if value:
        return value

    return str(default)


def interactive():
    print()
    print("CUT MODE")
    print("1 = isolate one node")
    print("2 = manual sij")
    print("3 = AUTO node1..node5")

    mode = ask(
        "Select",
        3
    )

    if mode == "1":
        cut_mode = "node"
        node = ask(
            "Node 1..5",
            1
        )
        sij = None

    elif mode == "2":
        cut_mode = "sij"
        node = None
        sij = ask(
            "Example s12,s31,s52",
            "s12"
        )

    else:
        cut_mode = "auto"
        node = None
        sij = None


    print()
    print("ANALYSIS")
    print("1 = bifurcation")
    print("2 = K-map")
    print("3 = both")


    analysis = ask(
        "Select",
        3
    )


    if analysis == "1":
        command = "bif"

    elif analysis == "2":
        command = "kmap"

    else:
        command = "all"


    argv = [
        command,
        "--cut-mode",
        cut_mode,
        "--workers",
        ask(
            "Workers",
            8
        )
    ]


    if node is not None:
        argv += [
            "--node",
            node
        ]


    if sij is not None:
        argv += [
            "--sij",
            sij
        ]


    if command in (
        "bif",
        "all"
    ):

        xaxis = ask(
            "Bif x-axis alpha2 or nu",
            "nu"
        )

        argv += [
            "--bif-x",
            xaxis
        ]

        if xaxis == "nu":

            argv += [
                "--alpha2",
                ask(
                    "Fixed alpha2",
                    -10
                )
            ]

            default_min = 0.1
            default_max = 0.9
            default_step = 0.05

        else:

            argv += [
                "--nu",
                ask(
                    "Fixed nu",
                    0.7
                )
            ]

            default_min = -10
            default_max = 10
            default_step = 0.5


        argv += [
            "--bif-min",
            ask(
                "Bif min",
                default_min
            ),

            "--bif-max",
            ask(
                "Bif max",
                default_max
            ),

            "--bif-step",
            ask(
                "Bif step",
                default_step
            ),

            "--bif-tmax",
            ask(
                "tMax",
                10000
            ),

            "--transient",
            ask(
                "Transient start",
                5000
            ),

            "--bif-point-size",
            ask(
                "Bif dot size",
                0.25
            )
        ]


    if command in (
        "kmap",
        "all"
    ):

        argv += [
            "--alpha-min",
            ask(
                "K alpha2 min",
                -10
            ),

            "--alpha-max",
            ask(
                "K alpha2 max",
                10
            ),

            "--alpha-step",
            ask(
                "K alpha2 step",
                1
            ),

            "--nu-min",
            ask(
                "K nu min",
                -0.1
            ),

            "--nu-max",
            ask(
                "K nu max",
                0.9
            ),

            "--nu-step",
            ask(
                "K nu step",
                0.1
            ),

            "--skip",
            ask(
                "0-1 skip",
                5000
            ),

            "--k-point-size",
            ask(
                "K-map dot size",
                2
            )
        ]


    print()
    print("Equivalent command:")

    print(
        "./dynamic_analysis.py "
        + " ".join(argv)
    )

    print()


    return (
        make_parser()
        .parse_args(argv)
    )


def validate(args):
    if (
        args.cut_mode == "node"
        and args.node is None
    ):
        raise SystemExit(
            "node mode needs --node"
        )

    if (
        args.cut_mode == "sij"
        and not args.sij
    ):
        raise SystemExit(
            "sij mode needs --sij"
        )


def main():
    if (
        not CASE_BIN.exists()
        or not TEST_BIN.exists()
    ):
        print(
            "Run ./build_dynamic.sh first"
        )

        return 1


    args = (
        make_parser()
        .parse_args()
    )


    if args.command == "interactive":
        args = interactive()


    validate(args)


    print()
    print(
        "Existing numerical core: "
        "NOT MODIFIED"
    )

    print(
        "Cuts are LOCAL copies only."
    )

    print(
        "Same command after reboot "
        "= resume."
    )


    for (
        label,
        cuts
    ) in cut_sets(args):

        print()
        print(
            "===================="
        )

        print(
            label,
            "cuts:",
            ",".join(cuts)
        )

        print(
            "===================="
        )


        if args.command in (
            "bif",
            "all"
        ):
            run_bif(
                args,
                label,
                cuts
            )


        if args.command in (
            "kmap",
            "all"
        ):
            run_kmap(
                args,
                label,
                cuts
            )


    print()
    print("DONE")
    print(
        "Results:",
        RESULTS
    )


    return 0


if __name__ == "__main__":
    raise SystemExit(
        main()
    )
