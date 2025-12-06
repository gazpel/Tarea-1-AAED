#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>

#define MAX_N 50
#define MAX_M 20

// Estructura de una operación
typedef struct {
    int proc;   // máquina/proceso donde se ejecuta (0..m-1)
    int t;      // tiempo de procesamiento
} Operacion;

// Cada carga tiene m operaciones
typedef struct {
    int n_ops;               // normalmente = m
    Operacion op[MAX_M];     // op[k] = operación k-ésima
} Carga;

// Variables globales del problema
int n, m;                    // número de cargas y procesos
Carga cargas[MAX_N];         // datos de cada carga

// Estado de la planificación parcial
int tiempoLibreProc[MAX_M];  // cuándo queda libre cada proceso p
int tiempoListoCarga[MAX_N]; // cuándo termina la última operación de cada carga j
int siguienteOp[MAX_N];      // índice de la próxima operación a ejecutar de cada carga j

int totalOps;                // n * m
int mejorTiempo;             // mejor makespan encontrado hasta ahora
long long nodosExplorados;   // contador de llamadas recursivas

// ---------------- LECTURA DE INSTANCIA ----------------

void leerInstancia(const char *nombre) {
    FILE *f = fopen(nombre, "r");
    if (!f) {
        perror("Error al abrir archivo");
        exit(EXIT_FAILURE);
    }

    if (fscanf(f, "%d %d", &n, &m) != 2) {
        fprintf(stderr, "Error leyendo n y m\n");
        fclose(f);
        exit(EXIT_FAILURE);
    }

    if (n > MAX_N || m > MAX_M) {
        fprintf(stderr, "Instancia excede los límites\n");
        fclose(f);
        exit(EXIT_FAILURE);
    }

    for (int j = 0; j < n; j++) {
        cargas[j].n_ops = m;
        for (int k = 0; k < m; k++) {
            int p, t;
            if (fscanf(f, "%d %d", &p, &t) != 2) {
                fprintf(stderr, "Error leyendo carga %d operación %d\n", j, k);
                fclose(f);
                exit(EXIT_FAILURE);
            }
            cargas[j].op[k].proc = p - 1;  // archivo usa 1..m, convertimos a 0..m-1
            cargas[j].op[k].t    = t;
        }
    }

    fclose(f);
    totalOps = n * m;
}

// ---------------- CÁLCULO MAKESPAN PARCIAL ----------------

int calcularTiempoActual() {
    int max = 0;
    for (int p = 0; p < m; p++) {
        if (tiempoLibreProc[p] > max) {
            max = tiempoLibreProc[p];
        }
    }
    return max;
}

// ---------------- ALGORITMO EXACTO (BACKTRACKING) ----------------

void explorar(int opsRealizadas) {
    nodosExplorados++;

    int tiempoActual = calcularTiempoActual();

    // Poda simple
    if (tiempoActual >= mejorTiempo) {
        return;
    }

    // Caso base: todas las operaciones programadas
    if (opsRealizadas == totalOps) {
        if (tiempoActual < mejorTiempo) {
            mejorTiempo = tiempoActual;
        }
        return;
    }

    // Para cada carga que tenga operaciones disponibles
    for (int j = 0; j < n; j++) {
        if (siguienteOp[j] < cargas[j].n_ops) {

            int k = siguienteOp[j];
            int p = cargas[j].op[k].proc;
            int t = cargas[j].op[k].t;

            // Respaldos
            int backupProc = tiempoLibreProc[p];
            int backupCarga = tiempoListoCarga[j];
            int backupSigOp = siguienteOp[j];

            // Calculamos inicio y término
            int inicio = (tiempoListoCarga[j] > tiempoLibreProc[p])
                        ? tiempoListoCarga[j]
                        : tiempoLibreProc[p];
            int termino = inicio + t;

            // Actualizamos estado
            tiempoListoCarga[j] = termino;
            tiempoLibreProc[p]  = termino;
            siguienteOp[j]      = k + 1;

            // Recursión
            explorar(opsRealizadas + 1);

            // Backtracking
            tiempoListoCarga[j] = backupCarga;
            tiempoLibreProc[p]  = backupProc;
            siguienteOp[j]      = backupSigOp;
        }
    }
}

// ---------------- PROGRAMA PRINCIPAL ----------------

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <archivo_instancia>\n", argv[0]);
        return EXIT_FAILURE;
    }

    leerInstancia(argv[1]);

    // Inicializamos el estado
    for (int p = 0; p < m; p++) tiempoLibreProc[p] = 0;
    for (int j = 0; j < n; j++) {
        tiempoListoCarga[j] = 0;
        siguienteOp[j] = 0;
    }

    mejorTiempo = INT_MAX;
    nodosExplorados = 0;

    clock_t t0 = clock();
    explorar(0);
    clock_t t1 = clock();
    double segundos = (double)(t1 - t0) / CLOCKS_PER_SEC;

    printf("Instancia: %s\n", argv[1]);
    printf("n = %d, m = %d\n", n, m);
    printf("Makespan óptimo (mejorTiempo) = %d\n", mejorTiempo);
    printf("Nodos explorados = %lld\n", nodosExplorados);
    printf("Tiempo de ejecucion = %.6f segundos\n", segundos);

    return 0;
}
