/*
 * Lab 12 — Speedup y eficiencia (CC3069 Computación Paralela y Distribuida)
 *
 * Un solo programa con 4 experimentos. Cada uno imprime CSV en stdout:
 *   modo,N,p,param,t_seq,t_par,t_parte_seq
 *
 *   t_seq        tiempo de la versión 100% secuencial (sin OpenMP) del MISMO trabajo
 *   t_par        tiempo de la versión OpenMP con p hilos
 *   t_parte_seq  dentro de t_par, cuánto tardó la parte que NO se paraleliza
 *
 * Uso:
 *   ./lab12 amdahl     [W=400000] [fp=0.90] [pmax=núcleos]   (fp = fracción paralelizable)
 *   ./lab12 suma       [pmax=núcleos]
 *   ./lab12 desbalance [static|dynamic|guided] [W=40000] [pmax=núcleos]
 *   ./lab12 gustafson  [W_por_hilo=100000] [W_seq=20000] [pmax=núcleos]
 *
 * Cada medición se repite REPS = 5 veces y se reporta la MEDIANA
 * (regla de clase: "repite y reporta la mediana").
 */
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ITERS 1000 /* costo de una "unidad de trabajo" (~1 µs) */
#define REPS 5

/* Evita que el compilador elimine o saque de los ciclos el trabajo medido. */
#define BARRERA() __asm__ volatile("" ::: "memory")

static int cmp_d(const void *a, const void *b) {
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}
static double mediana(const double *v) {
    double c[REPS];
    memcpy(c, v, sizeof c);
    qsort(c, REPS, sizeof(double), cmp_d);
    return c[REPS / 2];
}

static double checksum = 0.0; /* se imprime en stderr para que nada se optimice */
static volatile double semilla = 1.2345; /* volatile: el compilador no puede precalcular nada */

/* Una unidad de trabajo: cadena de operaciones dependientes (no vectorizable). */
static inline double unidad(double x, int iters) {
    for (int k = 0; k < iters; k++) x = x * 0.9999999 + 1e-7;
    return x;
}

/* ---------- Parte secuencial "pura": cada paso depende del anterior ---------- */
static double parte_secuencial(long ns) {
    double x = semilla;
    for (long i = 0; i < ns; i++) x = unidad(x, ITERS); /* x(i) necesita x(i-1) */
    return x;
}

/* ---------- Versión SECUENCIAL completa (sin pragmas) ---------- */
static double programa_seq(long ns, long np) {
    double acc = parte_secuencial(ns);
    for (long i = 0; i < np; i++) acc += unidad((double)i, ITERS);
    return acc;
}

/* ---------- Versión PARALELA: misma parte secuencial + parte paralela ---------- */
static double programa_par(long ns, long np, int p, double *t_parte_seq) {
    double t0 = omp_get_wtime();
    double acc = parte_secuencial(ns); /* esto NO se puede repartir */
    *t_parte_seq = omp_get_wtime() - t0;

    double suma = 0.0;
#pragma omp parallel for num_threads(p) reduction(+ : suma) schedule(static)
    for (long i = 0; i < np; i++) suma += unidad((double)i, ITERS);
    return acc + suma;
}

static double medir_seq(long ns, long np) {
    double v[REPS];
    for (int r = 0; r < REPS; r++) {
        double t0 = omp_get_wtime();
        checksum += programa_seq(ns, np);
        v[r] = omp_get_wtime() - t0;
    }
    return mediana(v);
}

static double medir_par(long ns, long np, int p, double *t_ps) {
    double v[REPS], vps[REPS];
    for (int r = 0; r < REPS; r++) {
        double t0 = omp_get_wtime();
        checksum += programa_par(ns, np, p, &vps[r]);
        v[r] = omp_get_wtime() - t0;
    }
    double m = mediana(v);
    for (int r = 0; r < REPS; r++)
        if (v[r] == m) { *t_ps = vps[r]; break; } /* parte secuencial de la corrida mediana */
    return m;
}

/* ===================== Experimento 1: Amdahl (problema fijo) ===================== */
static void exp_amdahl(long W, double fp, int pmax) {
    long np = (long)(W * fp + 0.5), ns = W - np; /* fp paralelizable, 1 - fp secuencial */
    double ts = medir_seq(ns, np);
    for (int p = 1; p <= pmax; p++) {
        double tps, tp = medir_par(ns, np, p, &tps);
        printf("amdahl,%ld,%d,fp=%.2f,%.6f,%.6f,%.6f\n", W, p, fp, ts, tp, tps);
        fflush(stdout);
    }
}

/* ============ Experimento 2: granularidad (sumar N números, como en clase) ============ */
static void exp_suma(int pmax) {
    long tamanos[] = {16, 1024, 65536, 1048576, 16777216};
    int nt = sizeof(tamanos) / sizeof(tamanos[0]);
    long nmax = tamanos[nt - 1];
    double *a = malloc(nmax * sizeof(double));
    if (!a) { fprintf(stderr, "sin memoria\n"); exit(1); }
    for (long i = 0; i < nmax; i++) a[i] = 1.0 / (double)(i + 1);

    int ps[64], nps = 0;
    for (int p = 1; p <= pmax; p *= 2) ps[nps++] = p;
    if (ps[nps - 1] != pmax) ps[nps++] = pmax;

    { double s = 0.0; for (long i = 0; i < nmax; i++) s += a[i]; checksum += s; } /* calentamiento */

    for (int k = 0; k < nt; k++) {
        long n = tamanos[k];
        long R = 50000000L / n; /* repetir para que el tiempo sea medible */
        if (R < 3) R = 3;
        if (R > 100000) R = 100000;

        double v[REPS];
        for (int rep = 0; rep < REPS; rep++) {
            double t0 = omp_get_wtime();
            for (long r = 0; r < R; r++) {
                double s = 0.0;
                for (long i = 0; i < n; i++) s += a[i];
                checksum += s;
                BARRERA();
            }
            v[rep] = (omp_get_wtime() - t0) / R;
        }
        double ts = mediana(v);
        for (int j = 0; j < nps; j++) {
            int p = ps[j];
            for (int rep = 0; rep < REPS; rep++) {
                double t0 = omp_get_wtime();
                for (long r = 0; r < R; r++) {
                    double s = 0.0;
#pragma omp parallel for num_threads(p) reduction(+ : s) schedule(static)
                    for (long i = 0; i < n; i++) s += a[i];
                    checksum += s;
                    BARRERA();
                }
                v[rep] = (omp_get_wtime() - t0) / R;
            }
            double tp = mediana(v);
            printf("suma,%ld,%d,R=%ld,%.9f,%.9f,0\n", n, p, R, ts, tp);
            fflush(stdout);
        }
    }
    free(a);
}

/* ============ Experimento 3: desbalance de carga (costo crece con i) ============ */
static inline int costo(long i, long W) { return 1 + (int)((2.0 * ITERS * i) / W); }

static void exp_desbalance(const char *sched, long W, int pmax) {
    omp_sched_t kind = omp_sched_static;
    int chunk = 0;
    if (strcmp(sched, "dynamic") == 0) { kind = omp_sched_dynamic; chunk = 64; }
    else if (strcmp(sched, "guided") == 0) { kind = omp_sched_guided; chunk = 0; }
    else if (strcmp(sched, "static") != 0) { fprintf(stderr, "schedule inválido: %s\n", sched); exit(1); }
    omp_set_schedule(kind, chunk);

    double v[REPS];
    for (int rep = 0; rep < REPS; rep++) {
        double t0 = omp_get_wtime(), acc = 0.0;
        for (long i = 0; i < W; i++) acc += unidad((double)i, costo(i, W));
        v[rep] = omp_get_wtime() - t0;
        checksum += acc;
    }
    double ts = mediana(v);
    for (int p = 1; p <= pmax; p++) {
        for (int rep = 0; rep < REPS; rep++) {
            double t0 = omp_get_wtime(), acc = 0.0;
#pragma omp parallel for num_threads(p) reduction(+ : acc) schedule(runtime)
            for (long i = 0; i < W; i++) acc += unidad((double)i, costo(i, W));
            v[rep] = omp_get_wtime() - t0;
            checksum += acc;
        }
        double tp = mediana(v);
        printf("desbalance,%ld,%d,%s,%.6f,%.6f,0\n", W, p, sched, ts, tp);
        fflush(stdout);
    }
}

/* ============ Experimento 4: Gustafson (el problema crece con p) ============ */
static void exp_gustafson(long w_por_hilo, long w_seq, int pmax) {
    for (int p = 1; p <= pmax; p++) {
        long np = w_por_hilo * p; /* más hilos => más trabajo paralelo */
        double ts = medir_seq(w_seq, np);
        double tps, tp = medir_par(w_seq, np, p, &tps);
        printf("gustafson,%ld,%d,Wseq=%ld,%.6f,%.6f,%.6f\n", w_seq + np, p, w_seq, ts, tp, tps);
        fflush(stdout);
    }
}

int main(int argc, char **argv) {
    int ncpu = omp_get_num_procs();
    if (argc < 2) {
        fprintf(stderr,
                "Uso:\n"
                "  %s amdahl     [W=400000] [fp=0.90] [pmax=%d]\n"
                "  %s suma       [pmax=%d]\n"
                "  %s desbalance [static|dynamic|guided] [W=40000] [pmax=%d]\n"
                "  %s gustafson  [W_por_hilo=100000] [W_seq=20000] [pmax=%d]\n",
                argv[0], ncpu, argv[0], ncpu, argv[0], ncpu, argv[0], ncpu);
        return 1;
    }
    printf("modo,N,p,param,t_seq,t_par,t_parte_seq\n");
    const char *modo = argv[1];
    if (strcmp(modo, "amdahl") == 0) {
        long W = argc > 2 ? atol(argv[2]) : 400000;
        double fp = argc > 3 ? atof(argv[3]) : 0.90;
        int pmax = argc > 4 ? atoi(argv[4]) : ncpu;
        exp_amdahl(W, fp, pmax);
    } else if (strcmp(modo, "suma") == 0) {
        int pmax = argc > 2 ? atoi(argv[2]) : ncpu;
        exp_suma(pmax);
    } else if (strcmp(modo, "desbalance") == 0) {
        const char *s = argc > 2 ? argv[2] : "static";
        long W = argc > 3 ? atol(argv[3]) : 40000;
        int pmax = argc > 4 ? atoi(argv[4]) : ncpu;
        exp_desbalance(s, W, pmax);
    } else if (strcmp(modo, "gustafson") == 0) {
        long wph = argc > 2 ? atol(argv[2]) : 100000;
        long wsq = argc > 3 ? atol(argv[3]) : 20000;
        int pmax = argc > 4 ? atoi(argv[4]) : ncpu;
        exp_gustafson(wph, wsq, pmax);
    } else {
        fprintf(stderr, "modo desconocido: %s\n", modo);
        return 1;
    }
    fprintf(stderr, "(checksum %.3e)\n", checksum);
    return 0;
}
