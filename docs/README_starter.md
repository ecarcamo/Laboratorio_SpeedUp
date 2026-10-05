# Lab 12 — Speedup y eficiencia (CC3069)

Archivos:
- `lab12_speedup.c` — programa con los 4 experimentos (no necesitas modificarlo salvo en el reto).
- `Makefile` — compila en Linux, WSL y macOS.
- `graficar.py` — calcula S, E, C y F y genera las gráficas a partir de los CSV.

Compilar:
    make            # Linux / WSL
    make            # macOS con: brew install libomp
    make CC=gcc-14  # macOS con: brew install gcc

Experimentos (cada uno imprime CSV; guárdalo con >):
    ./lab12 amdahl 400000 0.90 > amdahl_fp90.csv    # fp = fracción paralelizable
    ./lab12 suma > suma.csv
    ./lab12 desbalance static > desb_static.csv
    ./lab12 gustafson > gustafson.csv

Gráficas:
    python3 graficar.py *.csv

Cada medición se repite 5 veces y se reporta la mediana.
Consejos para medir bien: conecta la laptop a la corriente, cierra navegador/Teams/Spotify,
y no uses la computadora mientras corre un experimento.
