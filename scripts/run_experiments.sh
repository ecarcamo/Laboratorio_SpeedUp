#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "$0")/.." && pwd)"
raw_dir="$root_dir/results/raw"
log_dir="$root_dir/evidence/logs"
binary="$root_dir/src/lab12"
mkdir -p "$raw_dir" "$log_dir"

build() {
  make -C "$root_dir/src" 2>&1 | tee "$log_dir/make.log"
}

run_case() {
  local csv_name="$1"; shift
  local log_file="$log_dir/${csv_name%.csv}.log"
  echo "\$ ./lab12 $*" | tee "$log_file"
  local started_at=$SECONDS
  "$binary" "$@" 2>>"$log_file" | tee "$raw_dir/$csv_name" | tee -a "$log_file"
  echo "(${csv_name}: $((SECONDS - started_at)) s)" | tee -a "$log_file"
}

run_amdahl() {
  run_case amdahl_fp100.csv amdahl 400000 1.00
  run_case amdahl_fp95.csv amdahl 400000 0.95
  run_case amdahl_fp90.csv amdahl 400000 0.90
  run_case amdahl_fp75.csv amdahl 400000 0.75
  run_case amdahl_fp50.csv amdahl 400000 0.50
}

run_suma() {
  run_case suma.csv suma
}

run_desbalance() {
  run_case desb_static.csv desbalance static
  run_case desb_dynamic.csv desbalance dynamic
  run_case desb_guided.csv desbalance guided
}

run_gustafson() {
  run_case gustafson.csv gustafson
}

target="${1:-all}"
build
case "$target" in
  amdahl) run_amdahl ;;
  suma) run_suma ;;
  desbalance) run_desbalance ;;
  gustafson) run_gustafson ;;
  all) run_amdahl; run_suma; run_desbalance; run_gustafson ;;
  *) echo "uso: $0 [all|amdahl|suma|desbalance|gustafson]" >&2; exit 1 ;;
esac
