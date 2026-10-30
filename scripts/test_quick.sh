#!/usr/bin/env bash
# Tiered test entry. Policy and tier definitions: docs/TESTING.md.
#
#   scripts/test_quick.sh                  T0: host build + ctest + Python build/console checks
#   scripts/test_quick.sh -R <regex>       T0 focused on matching ctest names
#   scripts/test_quick.sh --asan           T0 + ASan/UBSan full host run (before commit/merge)
#   scripts/test_quick.sh --firmware       T1: + R1 local-frame firmware build (needs IDF v6.1 env)
#   scripts/test_quick.sh --firmware --all-profiles
#                                          T2: + original board and every Read Pico profile
#
# Never flashes, never touches a device.
set -euo pipefail

project_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$project_dir"

filter=""
run_asan=0
run_fw=0
all_profiles=0
while [[ $# -gt 0 ]]; do
    case "$1" in
        -R) filter="${2:?-R needs a regex}"; shift 2 ;;
        --asan) run_asan=1; shift ;;
        --firmware) run_fw=1; shift ;;
        --all-profiles) all_profiles=1; shift ;;
        -h|--help) sed -n '2,13p' "${BASH_SOURCE[0]}"; exit 0 ;;
        *) echo "unknown option: $1" >&2; exit 2 ;;
    esac
done

jobs="$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)"
cmake_idf=()
if [[ -n "${IDF_PATH:-}" ]]; then
    cmake_idf=(-DREAD_PICO_TEST_IDF_PATH="$IDF_PATH")
fi

step() { printf '\n== %s ==\n' "$*"; }

step "T0 host build + ctest"
cmake -S host_tests -B host_tests/build ${cmake_idf[@]+"${cmake_idf[@]}"} >/dev/null
cmake --build host_tests/build -j"$jobs"
ctest_args=(--test-dir host_tests/build --output-on-failure -j"$jobs")
[[ -n "$filter" ]] && ctest_args+=(-R "$filter")
ctest "${ctest_args[@]}"

step "T0 python build/console guards"
python3 host_tests/test_read_pico_build.py
python3 host_tests/test_read_pico_local_frame_build.py
python3 host_tests/test_read_pico_console.py

if [[ "$run_asan" -eq 1 ]]; then
    step "ASan/UBSan full host run"
    san='-fsanitize=address,undefined -fno-omit-frame-pointer'
    cmake -S host_tests -B host_tests/build-asan ${cmake_idf[@]+"${cmake_idf[@]}"} \
        -DCMAKE_C_FLAGS="$san" -DCMAKE_CXX_FLAGS="$san" \
        -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined' >/dev/null
    cmake --build host_tests/build-asan -j"$jobs"
    ctest --test-dir host_tests/build-asan --output-on-failure -j"$jobs"
fi

if [[ "$run_fw" -eq 1 ]]; then
    if ! command -v idf.py >/dev/null 2>&1; then
        echo "idf.py not found: activate the pinned ESP-IDF v6.1 first (docs/DEVELOPMENT.md)" >&2
        exit 1
    fi
    step "R1 local-frame firmware build"
    python3 build.py --board read_pico --step firmware --read-pico-local-frame
    if [[ "$all_profiles" -eq 1 ]]; then
        step "original board + remaining Read Pico profiles"
        python3 build.py --board waveshare_photopainter_73 --step firmware
        python3 build.py --board read_pico --step firmware
        python3 build.py --board read_pico --step firmware --read-pico-factory-diagnostic
        python3 build.py --board read_pico --step firmware --read-pico-factory-diagnostic --read-pico-display-validation
        python3 build.py --board read_pico --step firmware --read-pico-factory-diagnostic --read-pico-storage-validation
        python3 build.py --board read_pico --step firmware --read-pico-factory-diagnostic --read-pico-photoframe-integration
    fi
    for bin in build-read-pico-local-frame/esp32-photoframe.bin; do
        [[ -f "$bin" ]] && { ls -l "$bin"; shasum -a 256 "$bin"; }
    done
fi

step "done"
