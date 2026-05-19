#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <mpi.h>

// Verifica si es seguro poner una reina en tablero[fila] = col
int es_seguro(int *tablero, int fila, int col) {
    for (int i = 0; i < fila; i++) {
        // Si está en la misma columna o en la misma diagonal, no es seguro
        if (tablero[i] == col || abs(tablero[i] - col) == abs(i - fila)) {
            return 0;
        }
    }
    return 1;
}

// Backtracking secuencial estándar a partir de una fila dada
long int resolver_nreinas_rec(int *tablero, int N, int fila) {
    if (fila == N) {
        return 1; // Se encontró una solución válida
    }

    long int soluciones = 0;
    for (int col = 0; col < N; col++) {
        if (es_seguro(tablero, fila, col)) {
            tablero[fila] = col;
            soluciones += resolver_nreinas_rec(tablero, N, fila + 1);
        }
    }
    return soluciones;
}

double dwalltime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

int main(int argc, char *argv[]) {
    int N;
    double ini, fin;

    int id, nProcs;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &nProcs);

    if (argc < 2) {
        if (id == 0) {
            printf("Error. Usar: mpirun -np P %s <Dimension N>\n", argv[0]);
        }
        MPI_Finalize();
        exit(1);
    }

    N = atoi(argv[1]);

    if (id == 0) {
        printf("Calculando soluciones para %d-Reinas usando %d procesos...\n", N, nProcs);
        ini = dwalltime();
    }

    long int soluciones_locales = 0;
    
    // Cada proceso necesita su propio vector de tablero local para no pisarse
    int *tablero_local = (int *)malloc(N * sizeof(int));

    // --- PARALELISMO POR ENTRELAZADO CÍCLICO ---
    // Generamos las tareas fijando la primera reina (fila 0) en la columna 'c0'
    // Cada proceso toma las columnas de forma cíclica según su ID
    for (int c0 = id; c0 < N; c0 += nProcs) {
        tablero_local[0] = c0; // Fijamos primera reina
        
        // Llamamos al backtracking para que resuelva el resto del árbol desde la fila 1
        soluciones_locales += resolver_nreinas_rec(tablero_local, N, 1);
    }

    long int soluciones_globales = 0;

    // Reducción para sumar las soluciones encontradas por cada proceso
    MPI_Reduce(&soluciones_locales, &soluciones_globales, 1, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

    free(tablero_local);

    if (id == 0) {
        fin = dwalltime() - ini;
        printf("\n================ RESULTADOS ================\n");
        printf("N-Reinas: N = %d\n", N);
        printf("Total de soluciones encontradas: %ld\n", soluciones_globales);
        printf("Tiempo de computo paralelo:      %f segundos\n", fin);
        printf("============================================\n");
    }

    MPI_Finalize();
    return 0;
}