# Lab 12 — Speedup y eficiencia (CC3069)

Computación Paralela y Distribuida · Universidad del Valle de Guatemala

| Integrante | Carné |
|---|---|
| Esteban Cárcamo | 23016 |
| Nicolás Concuá | 23197 |

## Estructura

| Carpeta | Contenido |
|---|---|
| `docs/` | Enunciado del laboratorio y README original del starter |
| `src/` | `lab12_speedup.c` y `Makefile` (sin modificar) |
| `scripts/` | `graficar.py` del starter y scripts para correr, graficar y tabular |
| `results/raw/` | Los 10 CSV originales (5 amdahl, 1 suma, 3 desbalance, 1 gustafson) |
| `results/derived/` | Los `*_con_S_E.csv` generados por `graficar.py` y tablas auxiliares |
| `figures/` | Las 7 gráficas generadas por `graficar.py` |
| `evidence/` | Datos del equipo, salida de `make`, bitácoras de cada corrida y capturas en `screenshots/` |
| `report/` | Reporte en Markdown (base de `reporte_lab12.pdf`) |

## Cómo reproducir

```bash
./scripts/system_info.sh
./scripts/run_experiments.sh all
./scripts/make_plots.sh
python3 scripts/part1_table.py
```

`run_experiments.sh` acepta también `amdahl`, `suma`, `desbalance` o `gustafson` para correr una sola parte.

Para medir bien: laptop conectada a la corriente, apps cerradas y sin usar la computadora mientras corre.
