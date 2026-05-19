#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h> 
#include <mpi.h>

double* leerMatriz(double *m, int n, char * fullpath);
double dwalltime();

int main(int argc, char *argv[]){
    int N;
    double ini, fin;

    // 1. Inicializar el entorno MPI
    int id, nProcs;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &nProcs);

    // Chequeo de parámetros mínimos (Programa, N, Archivo)
    if (argc < 2) {
        if (id == 0) {
            printf("\nError en los parametros. Usar: mpirun -np P %s N <ruta y archivo matriz AN.m>\n", argv[0]);
        }
        MPI_Finalize();
        exit(1);
    }

    N = atoi(argv[1]);
    char* fileA = argv[2];

    int iteraciones = N / nProcs; // Cantidad de filas que procesa cada uno

    double *A = NULL;
    // Buffer local: cada proceso aloja solo la porción de filas que le toca
    double *mi_A = (double *) malloc(iteraciones * N * sizeof(double));

    if (id == 0) {
        A = (double *) malloc(N * N * sizeof(double));
        printf("Leyendo matriz %d x %d desde archivo...\n", N, N);
        A = leerMatriz(A, N, fileA);
        ini = dwalltime();
    }

    // 2. Distribuir las filas de la matriz equitativamente entre todos los procesos
    MPI_Scatter(A, iteraciones * N, MPI_DOUBLE, mi_A, iteraciones * N, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // --- CÓMPUTO LOCAL ---
    // Inicializamos las variables locales con el primer elemento de la porción local
    double min_local = mi_A[0];
    double max_local = mi_A[0];
    double suma_local = 0.0;
    int total_elementos_local = iteraciones * N;

    for (int i = 0; i < total_elementos_local; i++) {
        double val = mi_A[i];
        if (val < min_local) min_local = val;
        if (val > max_local) max_local = val;
        suma_local += val;
    }

    // Variables globales donde el Master va a recibir los resultados finales
    double min_global, max_global, suma_global;

    // 3. REDUCCIONES COLECTIVAS MPI
    // Buscamos el mínimo absoluto entre todos los min_local
    MPI_Reduce(&min_local, &min_global, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
    
    // Buscamos el máximo absoluto entre todos los max_local
    MPI_Reduce(&max_local, &max_global, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    
    // Sumamos todas las sumas locales para obtener el total general
    MPI_Reduce(&suma_local, &suma_global, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    // Liberamos los buffers locales en todos los procesos
    free(mi_A);

    // 4. El Master calcula el promedio final e informa los resultados
    if (id == 0) {
        fin = dwalltime() - ini;
        
        // El promedio es la suma de todos los elementos dividida la cantidad total de celdas (N * N)
        double promedio_global = suma_global / (double)(N * N);

        printf("\n================ RESULTADOS ================\n");
        printf("Matriz Analizada: %s (N = %d)\n", fileA, N);
        printf("Valor Minimo:     %f\n", min_global);
        printf("Valor Maximo:     %f\n", max_global);
        printf("Valor Promedio:   %f\n", promedio_global);
        printf("Tiempo de computo paralelo: %f segundos\n", fin);
        printf("============================================\n");

        free(A);
    }

    MPI_Finalize();
    return 0;
}

// --- FUNCIONES AUXILIARES ---
double* leerMatriz(double *m, int n, char * fullpath) {
    FILE* archivo = fopen(fullpath, "rb");
    if (!archivo) {
        perror("Error al abrir el archivo de matriz\n");
        return NULL;
    }
    fread(m, sizeof(double), n * n, archivo);
    fclose(archivo);
    return m;
}

double dwalltime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}