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

Comandos: `./lab12 amdahl 400000 {1.00,0.95,0.90,0.75,0.50}` → `results/raw/amdahl_fp*.csv`. Gráficas: `figures/amdahl_speedup.png` y `figures/amdahl_eficiencia.png`. Detalle de todas las filas en `results/derived/part1_table.md` y `results/derived/amdahl_con_S_E.csv`.

### Predicción frente a lo medido

| Caso | Predicción | Medido | ¿Por qué la diferencia? |
|---|---|---|---|
| fp = 1.00, p = 32 | ≈ 16 | **15.28** | Acertamos el orden. El máximo real no está en p = 32 sino en **p = 24 (S = 16.28)**: al pasar a 25 hilos empieza el Hyper-Threading y S cae a 12.81. |
| fp = 0.90, p = 32 | ≈ 6–7 | **5.91** | Un poco por debajo. La parte paralela pierde lo mismo que con fp = 1.00 (núcleos E y HT) y además la parte secuencial se vuelve un poco más lenta cuando hay muchos hilos (ver pregunta 6). |

### Tabla (pmax = 32)

| fp | S (p=2) | S (p=4) | S (pmax) | E (pmax) | p con F máxima | p con Tp mínimo |
|---|---|---|---|---|---|---|
| 1.00 | 1.95 | 3.77 | 15.28 | 0.477 | 23 | 24 |
| 0.95 | 1.90 | 3.31 | 8.38 | 0.262 | 23 | 23 |
| 0.90 | 1.80 | 2.74 | 5.91 | 0.185 | 8 | 22 |
| 0.75 | 1.55 | 2.21 | 3.30 | 0.103 | 3 | 22 |
| 0.50 | 1.40 | 1.63 | 1.84 | 0.058 | 1 | 22 |

### Cálculo a mano de S, E, C y F

Fórmulas de clase: S = Ts / Tp · E = S / p · C = p · Tp · F = S / C.

**Fila 1 — fp = 1.00, p = 4** (Ts = 0.504553 s, Tp = 0.133820 s)

- S = 0.504553 / 0.133820 = **3.770**
- E = 3.770 / 4 = **0.943**
- C = 4 × 0.133820 = **0.535 s** (hilo-segundos)
- F = 3.770 / 0.535 = **7.04 s⁻¹**

**Fila 2 — fp = 0.90, p = 32** (Ts = 0.501464 s, Tp = 0.084802 s)

- S = 0.501464 / 0.084802 = **5.913**
- E = 5.913 / 32 = **0.185**
- C = 32 × 0.084802 = **2.714 s**
- F = 5.913 / 2.714 = **2.18 s⁻¹**

**Fila 3 — fp = 0.90, p = 8** (Ts = 0.501464 s, Tp = 0.121345 s)

- S = 0.501464 / 0.121345 = **4.133**
- E = 4.133 / 8 = **0.517**
- C = 8 × 0.121345 = **0.971 s**
- F = 4.133 / 0.971 = **4.26 s⁻¹**

Los tres coinciden con `amdahl_con_S_E.csv` generado por `graficar.py`.

### Preguntas

**1. Con fp = 1.00, ¿llegaste a S = p?**

No. Con p = 32, S = 15.28 (E = 0.48), y el mejor valor fue S = 16.28 con p = 24. Las causas, en orden de peso:

| Causa | Evidencia en los datos | ¿Encaja en las 4 fugas? |
|---|---|---|
| **Hyper-Threading.** Desde p = 25, dos hilos comparten un núcleo P y cada uno va a la mitad de velocidad. Con `static` el equipo espera al hilo más lento. | Tp pasa de 0.0310 s (p = 24) a 0.0394 s (p = 25), un **27 % más lento con un hilo más**. E cae de 0.68 a 0.51. | No. Es un recurso del hardware compartido. Su efecto se parece a η_b (desbalance en tiempo, no en iteraciones). |
| **Núcleos de eficiencia.** Solo hay 8 núcleos P. Del hilo 9 al 24 los hilos caen en núcleos E (4.0 GHz, microarquitectura más simple). | E se mantiene en ≈ 0.80 hasta p = 8 y baja a ≈ 0.70 entre p = 9 y p = 24. | No. Los «procesadores» no son iguales; con `static` también se ve como η_b. |
| **Turbo boost con un solo hilo.** Ts se mide con un hilo que puede ir a 5.6 GHz. Con muchos núcleos activos, el límite de potencia y temperatura baja la frecuencia de todos. | Aunque con p = 8 todos los hilos tienen su propio núcleo P, E es solo 0.80. | No. La referencia Ts corre en un procesador «más rápido» que los que usa Tp. |
| **Crear y sincronizar hilos** (fork/join, barrera, `reduction`). | Pesa poco: Tp ≈ 30 ms frente a unos pocos µs de overhead. | Sí: η_c. |

**2. Forma de la curva de S con fp = 0.75**

Es cóncava y se aplana rápido hacia la asíntota 1/(1 − 0.75) = 4. Llega a S ≈ 3.0 con p = 12 y desde ahí oscila entre 2.9 y 3.3: el ruido de la parte secuencial (0.130–0.143 s) ya es mayor que lo que ahorra cada hilo nuevo.

- El hilo 2 aportó S(2) − S(1) = 1.547 − 0.976 = **+0.571**.
- El hilo 8 aportó S(8) − S(7) = 2.637 − 2.527 = **+0.110**, unas **5 veces menos**.

Cada hilo nuevo solo puede achicar la parte paralela, que ya es pequeña. Los 0.13 s secuenciales no cambian.

**3. Nuestro modelo (problema fijo)**

Del tiempo T1 en un núcleo, la parte (1 − fp)·T1 no se reparte y la parte fp·T1 se divide entre p hilos:

$$T_p = (1 - f_p)\,T_1 + \frac{f_p\,T_1}{p}$$

$$S(p) = \frac{T_1}{T_p} = \frac{T_1}{(1 - f_p)\,T_1 + \frac{f_p T_1}{p}} = \frac{1}{(1 - f_p) + \dfrac{f_p}{p}}$$

T1 desaparece: el speedup depende solo de la fracción paralelizable y del número de hilos.

**4. Predicción con fp = 0.90 y p = 32**

S = 1 / (0.10 + 0.90/32) = 1 / (0.10 + 0.028125) = 1 / 0.128125 = **7.80**. Medimos **5.91**, el **76 %** de lo que predice el modelo (24 % menos). La diferencia es lo que el modelo no ve: núcleos distintos, HT, turbo y overhead (pregunta 1).

**5. Límite cuando p → ∞**

fp/p → 0, así que S(∞) = 1/(1 − fp). Con fp = 0.90, **S máximo = 10**. Con 1000 núcleos: S = 1/(0.10 + 0.0009) = **9.91**. Aunque sobren núcleos, el programa nunca baja del 10 % secuencial.

**6. Despejar fp desde lo medido**

$$\frac{1}{S} = 1 - f_p + \frac{f_p}{p} \;\Rightarrow\; f_p = \frac{1 - 1/S}{1 - 1/p}$$

Con S = 5.913 y p = 32: fp = (1 − 0.16912) / (1 − 0.03125) = 0.83088 / 0.96875 = **0.858**.

Sí, da menos de 0.90. Visto con nuestro modelo, el programa se comporta como si el **4.2 % del trabajo fuera secuencial «de más»**. Esa diferencia no es η_f: es el resto de fugas que el modelo mete en el mismo saco.

- **η_c**: crear y sincronizar el equipo de hilos y combinar el `reduction`.
- **Desbalance por hardware**: núcleos E y HT con `static`.
- **La parte secuencial también empeora con muchos hilos**: `t_parte_seq` sube de 0.0524 s (p = 1) a 0.0555 s (p = 32), un 6 % más. Nuestra hipótesis: el chip se calienta con tantos núcleos activos, y los hilos de OpenMP siguen girando un rato en espera activa después de cada región paralela, así que el hilo principal pierde turbo.

Hacemos lo mismo para todos los fp:

| fp nominal | 1.00 | 0.95 | 0.90 | 0.75 | 0.50 |
|---|---|---|---|---|---|
| fp despejado (p = 32) | 0.965 | 0.909 | 0.858 | 0.720 | 0.472 |

Hasta con fp = 1.00 sale 0.965: el 3.5 % «secuencial» aparente es overhead y hardware.

**7. Efectividad con fp = 0.90**

F es máxima con **p = 8** (F = 4.26 s⁻¹, S = 4.13, E = 0.52). Prácticamente empata con p = 7 (F = 4.256). El menor tiempo se obtiene con **p = 22** (Tp = 0.0806 s, S = 6.22). **No es el mismo p.** Pasar de 8 a 22 hilos baja el tiempo de 0.121 s a 0.081 s (un 34 % menos), pero cuesta 2.75 veces más hilos y el costo C sube de 0.97 a 1.77 hilo-segundos.

Es lo mismo que en el caso de 16 números de clase, donde el óptimo de F fue p = 4 aunque más hilos seguían bajando un poco el tiempo. Después del «codo», cada hilo extra compra muy poca velocidad a precio de un hilo completo. Con nuestro modelo, el máximo teórico de F para fp = 0.90 está en p = fp/(1 − fp) = 9, que coincide con los 8 medidos.

**8. El jefe y el servidor de 64 núcleos**

Tarda 20 s, de los que 2 s son lectura secuencial → fp = 18/20 = **0.90**.

| p | Tp = 2 + 18/p (s) | S | E | C = p·Tp (s) | F = S/C (s⁻¹) |
|---|---|---|---|---|---|
| 1 | 20.00 | 1.00 | 1.00 | 20.0 | 0.050 |
| 4 | 6.50 | 3.08 | 0.77 | 26.0 | 0.118 |
| 8 | 4.25 | 4.71 | 0.59 | 34.0 | 0.138 |
| **9** | **4.00** | **5.00** | **0.56** | **36.0** | **0.139** |
| 16 | 3.13 | 6.40 | 0.40 | 50.0 | 0.128 |
| 64 | 2.28 | 8.77 | 0.14 | 146.0 | 0.060 |
| ∞ | 2.00 | 10.00 | → 0 | → ∞ | → 0 |

Le respondemos: «Con 64 núcleos el programa bajaría de 20 s a 2.28 s (S = 8.8), pero 55 de los 64 núcleos estarían de más: la eficiencia sería del 14 % y la efectividad la mitad que con 8–9 núcleos. Con **8–16 núcleos** ya tenemos S = 4.7–6.4 y nunca podremos bajar de 2 s (S = 10) mientras la lectura sea secuencial. Antes de comprar el servidor conviene atacar esos 2 s: un disco más rápido o leer el archivo por partes. Y según nuestras mediciones, el hardware real rinde menos que el modelo: con fp = 0.90 y 32 hilos obtuvimos 5.9 y no 7.8.»

---

## Parte 2 — ¿Vale la pena paralelizar algo pequeño?

### 2a. El Ts honesto

Datos: fila p = 1 de `amdahl_fp100.csv` → `t_seq = 0.504553 s` (sin OpenMP) y `t_par = 0.502151 s` (OpenMP con 1 hilo).

**Predicción:** OpenMP con 1 hilo sería 1–5 % más lento. **Medido:** fue **0.48 % más rápido**. La predicción falló en el signo, y la razón es la escala: el overhead de abrir una región paralela es de microsegundos y aquí el trabajo dura medio segundo, así que la diferencia queda dentro del ruido de medición.

**1. ¿Cuál es más rápido y por cuánto?**

(0.504553 − 0.502151) / 0.504553 = **0.48 %** a favor de la versión OpenMP con 1 hilo. En las otras cuatro corridas de amdahl, la misma comparación da entre −2.5 % y +0.9 %:

| fp | t_seq (s) | t_par, p = 1 (s) | Diferencia |
|---|---|---|---|
| 1.00 | 0.504553 | 0.502151 | OpenMP 0.48 % más rápido |
| 0.95 | 0.504368 | 0.499791 | OpenMP 0.91 % más rápido |
| 0.90 | 0.501464 | 0.501529 | OpenMP 0.01 % más lento |
| 0.75 | 0.511521 | 0.524324 | OpenMP 2.50 % más lento |
| 0.50 | 0.541656 | 0.540567 | OpenMP 0.20 % más rápido |

Con trabajos grandes son prácticamente iguales. Con trabajos chicos la diferencia se vuelve enorme: en `suma.csv`, con N = 1024 la versión OpenMP de 1 hilo tarda 551 ns frente a 380 ns de la secuencial (**45 % más lenta**), y con N = 16 tarda 155 ns frente a ≈ 2 ns.

**2. Usar como Ts el programa paralelo con 1 hilo**

El speedup sale **inflado**. La versión paralela con 1 hilo paga el overhead de OpenMP (crear el equipo, repartir, `reduction`, barrera) sin ganar nada a cambio, así que T1,par ≥ Ts y S = T1,par / Tp ≥ Ts / Tp. Es tramposo porque compara contra una versión secuencial artificialmente lenta: la pregunta honesta es cuánto mejora frente al mejor programa secuencial que existe. Con nuestros datos de N = 1024, ese truco inflaría todos los speedups un **45 %** (0.551 / 0.380 = 1.45).

### 2b. Sumar N números

Comando: `./lab12 suma` → `results/raw/suma.csv`. Gráfica: `figures/suma_speedup.png`.

| N | Ts | S (p=1) | S (p=2) | S (p=4) | S (p=8) | S (p=16) | S (p=32) |
|---|---|---|---|---|---|---|---|
| 16 | ≈ 2 ns | 0.013 | 0.0006 | 0.0004 | 0.0003 | 0.0002 | **0.0001** |
| 1 024 | 380 ns | 0.69 | 0.12 | 0.08 | 0.05 | 0.03 | 0.01 |
| 65 536 | 33.7 µs | 1.18 | 1.80 | **3.01** | 1.89 | 1.90 | 1.06 |
| 1 048 576 | 494 µs | 1.02 | 1.90 | 2.31 | 3.65 | **5.56** | 4.80 |
| 16 777 216 | 9.02 ms | 1.01 | 1.85 | 2.40 | 2.26 | 2.29 | 2.34 |

> El CSV imprime los tiempos con 9 decimales (resolución de 1 ns), así que para N = 16 el Ts de ≈ 2 ns tiene un error de hasta ±25 %. El orden de magnitud no cambia.

**Predicción frente a lo medido:** dijimos que lo paralelo empezaría a ganar en N ≈ 65 536. Se cumplió: con 1024 todavía pierde con cualquier p, y con 65 536 ya gana (S = 3.0 con p = 4). Lo que no previmos es que, con N = 65 536, usar 32 hilos casi empata con la versión secuencial (S = 1.06): más hilos no siempre es mejor.

**3. N = 16 y p = pmax**

S = 0.000000002 / 0.000029963 ≈ **0.0001**: la versión paralela con 32 hilos es **≈ 15 000 veces más lenta** (30 µs frente a 2 ns; entre ≈ 12 000 y 20 000 veces por la resolución de 1 ns).

**4. El modelo de dos fases de clase**

El modelo cuenta solo sumas: Tp = (16/4 − 1) + log₂4 = 5 sumas, y supone que **pasar un resultado parcial y sincronizar a los hilos es gratis** o cuesta lo mismo que una suma. En la máquina, una suma cuesta ≈ 0.1–0.2 ns, pero abrir la región paralela, repartir, combinar el `reduction` y esperar en la barrera cuesta ≈ 5 µs con p = 4: el equivalente a **decenas de miles de sumas**. La fuga es **η_c (comunicación y sincronización)**.

**5. Lo que OpenMP hace en cada `parallel for` además de sumar**

1. Despertar a los hilos del equipo (la primera vez, crearlos), que estaban dormidos o en espera activa.
2. Calcular el bloque de iteraciones que le toca a cada hilo (`schedule(static)`).
3. Crear la copia privada de `s` en cada hilo e inicializarla en 0.
4. Traer los datos de `a[]` a la caché de cada núcleo; si estaban en la caché de otro núcleo, moverlos.
5. Combinar las copias privadas en el `reduction` (cada hilo publica su parcial y se juntan de forma sincronizada).
6. Esperar a todos en la barrera implícita al final del ciclo.
7. Devolver los hilos a espera y seguir solo con el hilo principal.

Con N = 16, todo el tiempo se va en esos pasos: sumar toma ≈ 2 ns de los 5 300 ns que tarda con p = 4. Además, el overhead crece con p: 3.6 µs (p = 2), 5.3 µs (p = 4), 7.8 µs (p = 8), 12.5 µs (p = 16) y 30 µs (p = 32). Más hilos son más hilos que despertar y que esperar en la barrera, y con p > 8 entran núcleos E de otros clústeres.

**6. ¿Desde qué N conviene paralelizar?**

Sumar un número cuesta ≈ 0.5 ns (33.7 µs / 65 536). Paralelizar ahorra N · 0.5 ns · (1 − 1/p) y cuesta el overhead de la región paralela:

| p | Overhead medido (N = 16) | N mínimo para empatar ≈ overhead / (0.5 ns · (1 − 1/p)) |
|---|---|---|
| 2 | 3.6 µs | ≈ 14 000 |
| 4 | 5.3 µs | ≈ 14 000 |
| 32 | 30 µs | ≈ 62 000 |

En nuestro equipo conviene **desde N ≈ 10⁴ con pocos hilos y desde N ≈ 6·10⁴ con los 32**. Encaja con lo medido: con N = 65 536 y p = 32 obtuvimos S = 1.06, justo en el empate.

**Regla práctica:** un `parallel for` vale la pena cuando el trabajo de la región es mucho mayor que su overhead, unas **decenas de microsegundos de trabajo como mínimo** (≥ 10⁴–10⁵ operaciones simples). Si el trabajo es mediano, usar menos hilos que núcleos. Y si el ciclo se ejecuta muchas veces, abrir la región paralela una sola vez afuera.

**7. Con N grande, ¿llega S a p?**

No. Con N = 16 777 216 (128 MiB, no cabe en los 36 MiB de L3), S se estanca en **≈ 2.3–2.4 desde p = 4** y no mejora con 8, 16 ni 32 hilos. Cada número se lee de memoria y se usa para una sola suma, así que el límite es el **ancho de banda de la memoria RAM, que comparten todos los núcleos**:

- Secuencial: 134 MB / 9.02 ms ≈ **14.9 GB/s**.
- Paralelo (p = 4): 134 MB / 3.75 ms ≈ **35.8 GB/s**, el techo práctico de la memoria de esta laptop. Más hilos ya no pueden leer más rápido.

En cambio, con N = 1 048 576 (8 MiB, cabe en la L3 de 36 MiB, y con 16 hilos cada pedazo de 512 KiB cabe en la L2 de su núcleo) S llega a 5.56 con p = 16, porque las cachés dan mucho más ancho de banda que la RAM.

**¿Encaja en las cuatro fugas?** No de forma limpia. No es parte secuencial (η_f), ni desbalance (η_b), ni trabajo repetido (η_r), ni sincronización (η_c): los hilos no se esperan entre sí, compiten por un **recurso de hardware compartido** (el bus de memoria). Lo más parecido sería η_c si contamos el traer datos de memoria como «comunicación», pero el efecto lo explica el hardware, no el algoritmo. Lo dejamos para la síntesis como algo que no encaja.

---

## Parte 3 — Cuando a un hilo le toca más trabajo

_(Pendiente — Nicolás.)_

---

## Parte 4 — ¿Y si el problema crece con los núcleos?

_(Pendiente — Nicolás.)_

---

## Síntesis

_(Pendiente — Nicolás.)_
