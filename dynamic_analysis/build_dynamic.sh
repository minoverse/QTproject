#!/usr/bin/env bash

set -euo pipefail

HERE="$HOME/buttonnetwork/dynamic_analysis"
ROOT="$HOME/buttonnetwork"
BUILD="$HERE/build"

SOURCE_PRO="$ROOT/chaos_sweep/chaos_case.pro"
LOCAL_PRO="$HERE/dynamic_case.pro"

if [[ ! -f "$SOURCE_PRO" ]]; then

    echo "ERROR:"
    echo "$SOURCE_PRO not found."

    exit 1
fi

mkdir -p "$BUILD"

# COPY only.
# Original chaos_case.pro is untouched.
cp "$SOURCE_PRO" "$LOCAL_PRO"

python3 - <<'PY'
from pathlib import Path

p = Path(
    "/home/minb1/buttonnetwork/dynamic_analysis/dynamic_case.pro"
)

s = p.read_text()

s = s.replace(
    "chaos_case.cpp",
    "dynamic_case.cpp"
)

s = s.replace(
    "TARGET = chaos_case",
    "TARGET = dynamic_case"
)

s = s.replace(
    "TARGET=chaos_case",
    "TARGET=dynamic_case"
)

p.write_text(s)
PY

cd "$BUILD"

qmake6 "$LOCAL_PRO"

make -j"$(nproc)"

gcc \
    -O3 \
    -std=c11 \
    "$HERE/prof_test01_dynamic.c" \
    -lm \
    -o "$BUILD/test01_dynamic"

echo
echo "BUILD DONE"
echo
echo "dynamic_case:"
echo "$BUILD/dynamic_case"
echo
echo "test01:"
echo "$BUILD/test01_dynamic"
echo
echo "EXISTING CORE FILES WERE NOT MODIFIED."
