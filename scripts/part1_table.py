#!/usr/bin/env python3
import csv
import glob
import os
import sys

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RAW_DIR = os.path.join(ROOT_DIR, "results", "raw")
OUTPUT_FILE = os.path.join(ROOT_DIR, "results", "derived", "part1_table.md")


def load_rows(path):
    rows = []
    with open(path, newline="") as handle:
        for record in csv.DictReader(handle):
            p = int(record["p"])
            t_seq = float(record["t_seq"])
            t_par = float(record["t_par"])
            speedup = t_seq / t_par
            cost = p * t_par
            rows.append({
                "p": p,
                "t_seq": t_seq,
                "t_par": t_par,
                "t_seq_part": float(record["t_parte_seq"]),
                "S": speedup,
                "E": speedup / p,
                "C": cost,
                "F": speedup / cost,
                "fp": float(record["param"].split("=")[1]),
            })
    return sorted(rows, key=lambda row: row["p"])


def amdahl_speedup(fp, p):
    return 1.0 / ((1.0 - fp) + fp / p)


def inferred_fp(speedup, p):
    return (1.0 - 1.0 / speedup) / (1.0 - 1.0 / p)


def by_p(rows, p):
    return next(row for row in rows if row["p"] == p)


def main():
    paths = sorted(glob.glob(os.path.join(RAW_DIR, "amdahl_*.csv")))
    if not paths:
        sys.exit("No hay CSV de amdahl en results/raw")
    tables = sorted((load_rows(path) for path in paths), key=lambda rows: -rows[0]["fp"])

    lines = [
        "| fp | S (p=2) | S (p=4) | S (pmax) | E (pmax) | p con F máxima | p con Tp mínimo | S Amdahl (pmax) | fp despejado |",
        "|---|---|---|---|---|---|---|---|---|",
    ]
    for rows in tables:
        fp = rows[0]["fp"]
        last = rows[-1]
        best_f = max(rows, key=lambda row: row["F"])
        best_t = min(rows, key=lambda row: row["t_par"])
        lines.append(
            f"| {fp:.2f} | {by_p(rows, 2)['S']:.2f} | {by_p(rows, 4)['S']:.2f} | {last['S']:.2f} "
            f"| {last['E']:.3f} | {best_f['p']} | {best_t['p']} "
            f"| {amdahl_speedup(fp, last['p']):.2f} | {inferred_fp(last['S'], last['p']):.4f} |"
        )

    lines += ["", "Detalle por p (S, E, F) para cada fp:", ""]
    for rows in tables:
        fp = rows[0]["fp"]
        lines += [f"### fp = {fp:.2f}", "", "| p | t_par (s) | t_parte_seq (s) | S | E | C (s) | F (1/s) |", "|---|---|---|---|---|---|---|"]
        for row in rows:
            lines.append(
                f"| {row['p']} | {row['t_par']:.6f} | {row['t_seq_part']:.6f} | {row['S']:.3f} "
                f"| {row['E']:.3f} | {row['C']:.6f} | {row['F']:.4f} |"
            )
        lines.append("")

    os.makedirs(os.path.dirname(OUTPUT_FILE), exist_ok=True)
    with open(OUTPUT_FILE, "w") as handle:
        handle.write("\n".join(lines) + "\n")
    print("\n".join(lines[: len(tables) + 2]))
    print(f"-> {OUTPUT_FILE}")


if __name__ == "__main__":
    main()
