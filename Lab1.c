#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>

#define MAX_N 50   // máximo de jobs (cargas)
#define MAX_M 20   // máximo de máquinas (procesos)

typedef struct {
    int machine;  // índice de la máquina (0..m-1)
    int time;     // tiempo de procesamiento
} Operation;

typedef struct {
    int n_ops;           // normalmente = m
    Operation ops[MAX_M];
} Job;

// Variables globales del problema
int n, m;               // número de jobs y máquinas
Job jobs[MAX_N];        // datos de las operaciones

// Estado de la solución parcial
int machine_time[MAX_M];   // tiempo en que queda libre cada máquina
int job_time[MAX_N];       // tiempo en que termina la última operación de cada job
int next_op[MAX_N];        // índice de la próxima operación a ejecutar para cada job

int total_ops;             // número total de operaciones (n * m)
int best_makespan;         // mejor makespan encontrado (cota superior)
long long nodes_explored;  // contador de nodos del árbol de búsqueda

// ----------------- Lectura de instancia -----------------

void leer_instancia(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        perror("Error al abrir archivo");
        exit(EXIT_FAILURE);
    }

    if (fscanf(f, "%d %d", &n, &m) != 2) {
        fprintf(stderr, "Error de formato en la primera línea\n");
        fclose(f);
        exit(EXIT_FAILURE);
    }

    if (n > MAX_N || m > MAX_M) {
        fprintf(stderr, "Instancia excede límites MAX_N o MAX_M\n");
        fclose(f);
        exit(EXIT_FAILURE);
    }

    for (int j = 0; j < n; j++) {
        jobs[j].n_ops = m;
        for (int k = 0; k < m; k++) {
            int proc, t;
            if (fscanf(f, "%d %d", &proc, &t) != 2) {
                fprintf(stderr, "Error de formato leyendo job %d op %d\n", j, k);
                fclose(f);
                exit(EXIT_FAILURE);
            }
            // En el archivo las máquinas están numeradas desde 1,
            // las convertimos a índices 0..m-1
            jobs[j].ops[k].machine = proc - 1;
            jobs[j].ops[k].time = t;
        }
    }

    fclose(f);

    total_ops = n * m;
}

// ----------------- Función auxiliar -----------------

// Calcula el makespan actual como el máximo tiempo entre todas las máquinas
int calcular_makespan_actual() {
    int max = 0;
    for (int p = 0; p < m; p++) {
        if (machine_time[p] > max) {
            max = machine_time[p];
        }
    }
    return max;
}

// ----------------- Backtracking exacto -----------------

void backtrack(int ops_realizadas) {
    nodes_explored++;

    // Calculamos el tiempo actual mínimo posible de terminación
    int tiempoActual = calcular_makespan_actual();

    // Poda simple: si ya no podemos mejorar la mejor solución encontrada, cortamos
    if (tiempoActual >= best_makespan) {
        return;
    }

    // Caso base: si ya programamos todas las operaciones, actualizamos la mejor solución
    if (ops_realizadas == total_ops) {
        if (tiempoActual < best_makespan) {
            best_makespan = tiempoActual;
        }
        return;
    }

    // Construimos el conjunto de jobs que aún tienen operaciones disponibles
    for (int j = 0; j < n; j++) {
        if (next_op[j] < jobs[j].n_ops) {
            int k = next_op[j];
            Operation op = jobs[j].ops[k];
            int p = op.machine;  // máquina
            int t = op.time;     // duración

            // Respaldo del estado antes de decidir
            int backup_machine = machine_time[p];
            int backup_job     = job_time[j];
            int backup_next    = next_op[j];

            // Programar la operación j-k en la máquina p
            int inicio  = (job_time[j] > machine_time[p]) ? job_time[j] : machine_time[p];
            int termino = inicio + t;

            job_time[j]      = termino;
            machine_time[p]  = termino;
            next_op[j]       = next_op[j] + 1;

            // Llamada recursiva
            backtrack(ops_realizadas + 1);

            // Backtracking: restaurar el estado
            job_time[j]      = backup_job;
            machine_time[p]  = backup_machine;
            next_op[j]       = backup_next;
        }
    }
}

// ----------------- Programa principal -----------------

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <archivo_instancia>\n", argv[0]);
        return EXIT_FAILURE;
    }

    leer_instancia(argv[1]);

    // Inicializar estado
    for (int p = 0; p < m; p++) {
        machine_time[p] = 0;
    }
    for (int j = 0; j < n; j++) {
        job_time[j] = 0;
        next_op[j]  = 0;
    }

    best_makespan = INT_MAX;
    nodes_explored = 0;

    clock_t t0 = clock();

    // Ejecutar backtracking exacto
    backtrack(0);

    clock_t t1 = clock();
    double secs = (double)(t1 - t0) / CLOCKS_PER_SEC;

    printf("Instancia: %s\n", argv[1]);
    printf("n = %d, m = %d\n", n, m);
    printf("Mejor makespan encontrado (C_best) = %d\n", best_makespan);
    printf("Nodos explorados = %lld\n", nodes_explored);
    printf("Tiempo de ejecucion = %.6f segundos\n", secs);

    return 0;
}
