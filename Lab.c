#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>

#define MAX_N 50      // máximo de jobs (cargas)
#define MAX_M 20      // máximo de máquinas (procesos)

typedef struct {
    int machine;   // índice de la máquina (0..m-1)
    int time;      // tiempo de procesamiento
} Operation;

typedef struct {
    int n_ops;             // normalmente = m
    Operation ops[MAX_M];  // operaciones del job
} Job;

// Datos globales del problema
int n, m;
Job jobs[MAX_N];

// Estado del algoritmo exacto
int machine_time[MAX_M];  // tiempo en que queda libre cada máquina
int job_time[MAX_N];      // tiempo de término de la última operación del job
int next_op[MAX_N];       // índice de la próxima operación por job

int total_ops;            // n * m
int C_best = INT_MAX;     // mejor makespan encontrado (upper bound)
long long nodes_explored = 0; // contador de nodos (para info)

// Función auxiliar
int max_int(int a, int b) {
    return (a > b) ? a : b;
}

// Lectura de la instancia desde archivo
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
            jobs[j].ops[k].machine = proc - 1; // convertimos a índice 0..m-1
            jobs[j].ops[k].time = t;
        }
    }

    fclose(f);

    total_ops = n * m;
}

// Obtiene el conjunto de jobs que tienen operación disponible
// A: arreglo de índices de jobs
// numA: cantidad de elementos en A
void obtener_disponibles(int *A, int *numA) {
    *numA = 0;
    for (int j = 0; j < n; j++) {
        if (next_op[j] < jobs[j].n_ops) {
            A[*numA] = j;
            (*numA)++;
        }
    }
}

// Programar una operación: actualiza machine_time, job_time, next_op
// y guarda los valores previos para poder deshacer
void programar_operacion(int j,
                         int *backup_machine_time,
                         int *backup_job_time,
                         int *backup_next_op) {
    int k = next_op[j];
    Operation op = jobs[j].ops[k];
    int p = op.machine;
    int t = op.time;

    *backup_machine_time = machine_time[p];
    *backup_job_time     = job_time[j];
    *backup_next_op      = next_op[j];

    int inicio  = max_int(job_time[j], machine_time[p]);
    int termino = inicio + t;

    job_time[j]     = termino;
    machine_time[p] = termino;
    next_op[j]      = next_op[j] + 1;
}

// Deshacer la última operación programada del job j
void deshacer_operacion(int j,
                        int backup_machine_time,
                        int backup_job_time,
                        int backup_next_op) {
    int k = backup_next_op; // índice de la operación que se había ejecutado
    int p = jobs[j].ops[k].machine;

    machine_time[p] = backup_machine_time;
    job_time[j]     = backup_job_time;
    next_op[j]      = backup_next_op;
}

// Cálculo del lower bound
int lower_bound() {
    int LB1, LB2, LB3;

    // LB1: makespan actual mínimo
    int C_actual = 0;
    for (int p = 0; p < m; p++) {
        if (machine_time[p] > C_actual)
            C_actual = machine_time[p];
    }
    LB1 = C_actual;

    // LB2: tiempos restantes por job
    int max_job_bound = 0;
    for (int j = 0; j < n; j++) {
        int restante_job = 0;
        for (int k = next_op[j]; k < jobs[j].n_ops; k++) {
            restante_job += jobs[j].ops[k].time;
        }
        int bound_j = job_time[j] + restante_job;
        if (bound_j > max_job_bound)
            max_job_bound = bound_j;
    }
    LB2 = max_job_bound;

    // LB3: tiempos restantes por máquina
    int max_mach_bound = 0;
    for (int p = 0; p < m; p++) {
        int restante_mach = 0;
        for (int j = 0; j < n; j++) {
            for (int k = next_op[j]; k < jobs[j].n_ops; k++) {
                if (jobs[j].ops[k].machine == p) {
                    restante_mach += jobs[j].ops[k].time;
                }
            }
        }
        int bound_p = machine_time[p] + restante_mach;
        if (bound_p > max_mach_bound)
            max_mach_bound = bound_p;
    }
    LB3 = max_mach_bound;

    int LB = max_int(LB1, max_int(LB2, LB3));
    return LB;
}

// Heurística inicial sencilla (SPT sobre operaciones disponibles) para obtener C_best
void heuristica_inicial() {
    int MT[MAX_M];  // tiempos de máquina (copia local)
    int JT[MAX_N];  // tiempos de job (copia local)
    int NO[MAX_N];  // next_op local

    for (int p = 0; p < m; p++)
        MT[p] = 0;
    for (int j = 0; j < n; j++) {
        JT[j] = 0;
        NO[j] = 0;
    }

    int ops_realizadas = 0;

    while (ops_realizadas < total_ops) {
        int A[MAX_N];
        int numA = 0;

        // construir operaciones disponibles
        for (int j = 0; j < n; j++) {
            if (NO[j] < jobs[j].n_ops) {
                A[numA] = j;
                numA++;
            }
        }

        // elegir la disponible con menor tiempo de procesamiento (SPT)
        int mejor_j = A[0];
        int mejor_t = jobs[mejor_j].ops[ NO[mejor_j] ].time;

        for (int i = 1; i < numA; i++) {
            int j = A[i];
            int t = jobs[j].ops[ NO[j] ].time;
            if (t < mejor_t) {
                mejor_t = t;
                mejor_j = j;
            }
        }

        int k = NO[mejor_j];
        Operation op = jobs[mejor_j].ops[k];
        int p = op.machine;
        int t = op.time;

        int inicio  = (JT[mejor_j] > MT[p]) ? JT[mejor_j] : MT[p];
        int termino = inicio + t;

        JT[mejor_j] = termino;
        MT[p]       = termino;
        NO[mejor_j] = NO[mejor_j] + 1;

        ops_realizadas++;
    }

    int C_heur = 0;
    for (int p = 0; p < m; p++) {
        if (MT[p] > C_heur)
            C_heur = MT[p];
    }

    C_best = C_heur;
}

// Ordenar las operaciones disponibles A[0..numA-1] según una heurística
// Aquí: orden decreciente por tiempo de procesamiento de su siguiente operación
void ordenar_disponibles(int *A, int numA) {
    for (int i = 0; i < numA - 1; i++) {
        for (int j = i + 1; j < numA; j++) {
            int job_i = A[i];
            int job_j = A[j];
            int ti = jobs[job_i].ops[ next_op[job_i] ].time;
            int tj = jobs[job_j].ops[ next_op[job_j] ].time;

            // Si queremos probar otra heurística, se puede cambiar el criterio:
            // Por ejemplo, decreciente por tiempo:
            if (tj > ti) {
                int temp = A[i];
                A[i] = A[j];
                A[j] = temp;
            }
        }
    }
}

// Branch & Bound recursivo
void BnB(int scheduled_ops) {
    nodes_explored++;

    // Caso base: todas las operaciones programadas
    if (scheduled_ops == total_ops) {
        int C = 0;
        for (int p = 0; p < m; p++) {
            if (machine_time[p] > C)
                C = machine_time[p];
        }
        if (C < C_best) {
            C_best = C;
            // aquí se podría guardar la mejor secuencia, si se desea
        }
        return;
    }

    // Lower Bound
    int LB = lower_bound();
    if (LB >= C_best) {
        // poda
        return;
    }

    int A[MAX_N];
    int numA = 0;
    obtener_disponibles(A, &numA);

    // Ordenar disponibles (heurística)
    ordenar_disponibles(A, numA);

    // Ramificación
    for (int i = 0; i < numA; i++) {
        int j = A[i];

        int backup_machine_time;
        int backup_job_time;
        int backup_next_op;

        programar_operacion(j,
                            &backup_machine_time,
                            &backup_job_time,
                            &backup_next_op);

        BnB(scheduled_ops + 1);

        deshacer_operacion(j,
                           backup_machine_time,
                           backup_job_time,
                           backup_next_op);
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Uso: %s archivo_in\n", argv[0]);
        printf("Ejemplo: %s procesamiento_3_2.IN\n", argv[0]);
        return 1;
    }

    leer_instancia(argv[1]);

    // Inicializar estado global
    for (int p = 0; p < m; p++)
        machine_time[p] = 0;
    for (int j = 0; j < n; j++) {
        job_time[j] = 0;
        next_op[j]  = 0;
    }

    // C_best muy grande al inicio
    C_best = INT_MAX;

    // Medición de tiempo
    clock_t t0 = clock();

    // Heurística inicial para obtener cota superior
    heuristica_inicial();

    // Ejecutar Branch & Bound desde el estado vacío
    BnB(0);

    clock_t t1 = clock();
    double secs = (double)(t1 - t0) / CLOCKS_PER_SEC;

    printf("Instancia: %s\n", argv[1]);
    printf("n = %d, m = %d\n", n, m);
    printf("Mejor makespan encontrado (C_best) = %d\n", C_best);
    printf("Nodos explorados = %lld\n", nodes_explored);
    printf("Tiempo de ejecucion = %.6f segundos\n", secs);

    return 0;
}
