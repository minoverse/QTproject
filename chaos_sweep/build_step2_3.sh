#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p build
cd build
qmake6 ../chaos_case.pro
make -j"$(nproc)"
gcc -O3 -march=native -DNDEBUG ../prof_test01.c -o test01 -lm
printf '\nBuild complete:\n  %s\n  %s\n' "$PWD/chaos_case" "$PWD/test01"
