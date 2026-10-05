#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "$0")/.." && pwd)"
derived_dir="$root_dir/results/derived"
figures_dir="$root_dir/figures"
mkdir -p "$derived_dir" "$figures_dir"

cd "$derived_dir"
python3 "$root_dir/scripts/graficar.py" "$root_dir"/results/raw/*.csv
mv -f ./*.png "$figures_dir/"
ls "$figures_dir"
