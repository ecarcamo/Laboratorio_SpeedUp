#!/usr/bin/env python3
"""
Lab 12 — Gráficas de speedup y eficiencia.

Uso:
    python3 graficar.py amdahl_*.csv          # uno o varios CSV del mismo modo o de varios
    python3 graficar.py *.csv

Para cada modo genera <modo>_speedup.png y <modo>_eficiencia.png, y un
<modo>_con_S_E.csv con las columnas agregadas (las mismas de la clase):
    S = Ts / Tp   E = S / p   C = p * Tp   F = S / C
Requiere: matplotlib (pip install matplotlib).
"""
import csv
import sys
from collections import defaultdict

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt


def leer(archivos):
    filas = []
    for a in archivos:
        with open(a, newline="") as f:
            for r in csv.DictReader(f):
                r["N"] = int(r["N"])
                r["p"] = int(r["p"])
                for k in ("t_seq", "t_par", "t_parte_seq"):
                    r[k] = float(r[k])
                r["S"] = r["t_seq"] / r["t_par"]
                r["E"] = r["S"] / r["p"]
                r["C"] = r["p"] * r["t_par"]
                r["F"] = r["S"] / r["C"]
                filas.append(r)
    return filas


def guardar_csv(modo, filas):
    cols = ["modo", "N", "p", "param", "t_seq", "t_par", "t_parte_seq", "S", "E", "C", "F"]
    with open(f"{modo}_con_S_E.csv", "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=cols, extrasaction="ignore")
        w.writeheader()
        for r in filas:
            w.writerow({**r, "S": f"{r['S']:.3f}", "E": f"{r['E']:.3f}",
                        "C": f"{r['C']:.6g}", "F": f"{r['F']:.4g}"})


def graficar_por_p(modo, filas, titulo_extra=""):
    """Series = valor de 'param' (fs, schedule, ...); eje x = p."""
    series = defaultdict(list)
    for r in filas:
        series[r["param"]].append(r)
    pmax = max(r["p"] for r in filas)

    for metrica, ylabel, ideal in (("S", "Speedup S = Ts / Tp", lambda p: p),
                                   ("E", "Eficiencia E = S / p", lambda p: 1.0)):
        plt.figure(figsize=(7, 4.5))
        ps = list(range(1, pmax + 1))
        plt.plot(ps, [ideal(p) for p in ps], "k--", lw=1, label="ideal")
        for nombre, rs in sorted(series.items()):
            rs.sort(key=lambda r: r["p"])
            plt.plot([r["p"] for r in rs], [r[metrica] for r in rs], "o-", label=str(nombre))
        plt.xlabel("Hilos (p)")
        plt.ylabel(ylabel)
        plt.title(f"{modo}{titulo_extra}")
        plt.grid(alpha=0.3)
        plt.legend()
        plt.tight_layout()
        nombre_png = f"{modo}_{'speedup' if metrica == 'S' else 'eficiencia'}.png"
        plt.savefig(nombre_png, dpi=150)
        plt.close()
        print("  ->", nombre_png)


def graficar_suma(filas):
    """Eje x = N (log); una serie por p."""
    series = defaultdict(list)
    for r in filas:
        series[r["p"]].append(r)
    plt.figure(figsize=(7, 4.5))
    plt.axhline(1.0, color="k", ls="--", lw=1, label="S = 1 (igual que secuencial)")
    for p, rs in sorted(series.items()):
        rs.sort(key=lambda r: r["N"])
        plt.plot([r["N"] for r in rs], [r["S"] for r in rs], "o-", label=f"p = {p}")
    plt.xscale("log", base=2)
    plt.xlabel("Cantidad de números a sumar (N, escala log)")
    plt.ylabel("Speedup S = Ts / Tp")
    plt.title("suma: speedup vs tamaño del problema")
    plt.grid(alpha=0.3, which="both")
    plt.legend()
    plt.tight_layout()
    plt.savefig("suma_speedup.png", dpi=150)
    plt.close()
    print("  -> suma_speedup.png")


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)
    filas = leer(sys.argv[1:])
    por_modo = defaultdict(list)
    for r in filas:
        por_modo[r["modo"]].append(r)

    for modo, rs in por_modo.items():
        print(f"[{modo}] {len(rs)} filas")
        guardar_csv(modo, rs)
        if modo == "suma":
            graficar_suma(rs)
        elif modo == "gustafson":
            graficar_por_p(modo, rs, " (problema crece con p)")
        else:
            graficar_por_p(modo, rs)


if __name__ == "__main__":
    main()
