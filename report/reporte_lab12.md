# Lab 12 — ¿Por qué 8 núcleos no son 8× más rápido?

**CC3069 Computación Paralela y Distribuida** · Universidad del Valle de Guatemala

| Integrante | Carné |
|---|---|
| Esteban Cárcamo | 23016 |
| Nicolás Concuá | 23197 |

---

## 0. Datos del equipo

Todas las mediciones se hicieron en la misma laptop, conectada a la corriente.

| Dato | Valor |
|---|---|
| CPU | 13th Gen Intel Core i9-13980HX (Raptor Lake, x86_64) |
| Núcleos físicos | 24 |
| Núcleos lógicos (hilos) | 32 → `pmax = 32` |
| Núcleos de rendimiento (P) | 8, con Hyper-Threading → 16 hilos (CPU 0–15), hasta 5.4–5.6 GHz |
| Núcleos de eficiencia (E) | 16, sin Hyper-Threading → 16 hilos (CPU 16–31), hasta 4.0 GHz |
| Hyper-Threading / SMT | Sí, solo en los núcleos P (2 hilos por núcleo P) |
| Caché | L1d 48 KiB por núcleo P / 32 KiB por núcleo E · L2 32 MiB (12 instancias) · L3 36 MiB compartida |
| Sistema operativo | Linux 7.2.6 (CachyOS), gobernador `powersave` |
| Compilador | GCC 16.2.1, `-O2 -Wall -fopenmp` |

Fuente: `lscpu` y `lscpu -e=CPU,CORE,MAXMHZ` (`evidence/system_info.txt`). Los núcleos P aparecen como `CORE` 0–7 con dos CPU cada uno; los núcleos E como `CORE` 8–23 con una CPU cada uno.

> **Consecuencia para todo el lab:** los 32 hilos no son iguales. 16 son hilos de núcleos P que comparten núcleo de dos en dos (HT) y 16 son núcleos E más lentos. Un hilo solo corre en un núcleo P con turbo a 5.6 GHz. Con `schedule(static)`, cada hilo recibe la misma cantidad de trabajo, así que el tiempo lo marca el hilo más lento.

---

## Predicciones (escritas antes de correr cualquier experimento)

| Parte | Pregunta | Predicción |
|---|---|---|
| 1 | S(32) con fp = 1.00 | **≈ 16**. No 32: con `static` el hilo más lento manda, y un núcleo E (4.0 GHz frente a 5.6 GHz de un P con turbo) o un hilo HT que comparte núcleo rinde más o menos la mitad que un hilo solo. 32 × ~0.5 ≈ 16. |
| 1 | S(32) con fp = 0.90 | **≈ 6–7**. Con el 10 % secuencial el tope teórico es 1 / (0.1 + 0.9/32) ≈ 7.8, y le restamos lo que pierde la parte paralela por los núcleos E y HT. |
| 2a | `t_seq` contra `t_par` con p = 1 | La versión con OpenMP y 1 hilo será **un poco más lenta (≈ 1–5 %)** por crear el equipo de hilos y hacer el `reduction`. |
| 2b | N desde el que conviene paralelizar | **N ≈ 65 536 (2¹⁶)**. Con 16 o 1024 números, despertar y sincronizar los hilos cuesta más que la suma. |
| 3 | Con p = 4 y `static`, fracción del trabajo del último hilo y S | El costo crece como una recta, así que el último cuarto del rango es un trapecio de área 1 − (3/4)² = **7/16 ≈ 43.8 %** del trabajo. S ≈ 16/7 ≈ **2.3** en lugar de 4. |
| 4 | S máximo para fp = 0.83 según el modelo de la Parte 1, y si pasará lo mismo aquí | Modelo de la Parte 1: S∞ = 1 / 0.17 ≈ **5.9** (con p = 32, ≈ 5.1). **Creemos que aquí NO pasará**: como el trabajo paralelo crece con p y la parte secuencial queda fija, el speedup debería crecer casi en línea recta con p (muy por encima de 6). |

---

## Parte 1 — La parte que no se puede repartir

_(Se completa después de medir.)_

---

## Parte 2 — ¿Vale la pena paralelizar algo pequeño?

_(Se completa después de medir.)_

---

## Parte 3 — Cuando a un hilo le toca más trabajo

_(Pendiente — Nicolás.)_

---

## Parte 4 — ¿Y si el problema crece con los núcleos?

_(Pendiente — Nicolás.)_

---

## Síntesis

_(Pendiente — Nicolás.)_
