#!/usr/bin/env python3
"""Parte 4: fracción secuencial s = t_parte_seq / t_par y modelo de Gustafson.

Lee results/raw/gustafson.csv y escribe results/derived/gustafson_s.md con,
para cada p: t_seq, t_par, t_parte_seq, S medido, s y S = s + p(1 - s).
"""
import csv
import os

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
INPUT_FILE = os.path.join(ROOT_DIR, "results", "raw", "gustafson.csv")
OUTPUT_FILE = os.path.join(ROOT_DIR, "results", "derived", "gustafson_s.md")


def gustafson_speedup(s, p):
    return s + p * (1.0 - s)


def load_rows(path):
    rows = []
    with open(path, newline="") as handle:
        for record in csv.DictReader(handle):
            p = int(record["p"])
            t_seq = float(record["t_seq"])
            t_par = float(record["t_par"])
            t_seq_part = float(record["t_parte_seq"])
            s = t_seq_part / t_par
            speedup = t_seq / t_par
            model = gustafson_speedup(s, p)
            rows.append({
                "p": p,
                "t_seq": t_seq,
                "t_par": t_par,
                "t_seq_part": t_seq_part,
                "S": speedup,
                "s": s,
                "model": model,
                "ratio": speedup / model,
            })
    return sorted(rows, key=lambda row: row["p"])


def main():
    rows = load_rows(INPUT_FILE)
    lines = [
        "| p | t_seq (s) | t_par (s) | t_parte_seq (s) | S medido | s | S modelo = s + p(1 − s) | medido / modelo |",
        "|---|---|---|---|---|---|---|---|",
    ]
    for row in rows:
        lines.append(
            f"| {row['p']} | {row['t_seq']:.3f} | {row['t_par']:.3f} | {row['t_seq_part']:.4f} "
            f"| {row['S']:.2f} | {row['s']:.3f} | {row['model']:.2f} | {row['ratio']:.2f} |"
        )
    with open(OUTPUT_FILE, "w") as handle:
        handle.write("\n".join(lines) + "\n")
    print("\n".join(lines))
    print(f"\nEscrito en {os.path.relpath(OUTPUT_FILE, ROOT_DIR)}")


if __name__ == "__main__":
    main()
