#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "$0")/.." && pwd)"
output_file="$root_dir/evidence/system_info.txt"

{
  echo "== date"; date -Iseconds
  echo; echo "== uname -a"; uname -a
  echo; echo "== lscpu"; LC_ALL=C lscpu
  echo; echo "== lscpu -e=CPU,CORE,MAXMHZ"; lscpu -e=CPU,CORE,MAXMHZ
  echo; echo "== nproc"; nproc
  echo; echo "== gcc --version"; gcc --version | head -1
  echo; echo "== scaling_governor"; sort /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor | uniq -c
  echo; echo "== power_supply online"
  for supply in /sys/class/power_supply/*; do
    [ -f "$supply/online" ] && echo "$(basename "$supply"): $(cat "$supply/online")"
  done
} > "$output_file"

echo "-> $output_file"
