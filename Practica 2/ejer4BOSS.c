#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h> 
#include <omp.h>

double dwalltime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

int es_seguro(int *tablero, int fila, int col) {
    for (int i = 0; i < fila; i++) {
        if (tablero[i] == col || abs(tablero[i] - col) == abs(i - fila)) {
            return 0;
        }
    }
    return 1;
}

// 1. Estilo LOLO: Retorna la suma directamente
long resolver(int *tablero, int fila, int N) {
    if (fila == N) return 1;

    long soluciones = 0;
    for (int col = 0; col < N; col++) {
        if (es_seguro(tablero, fila, col)) {
            tablero[fila] = col;
            soluciones += resolver(tablero, fila + 1, N);
        }
    }
    return soluciones;
}

int main(int argc, char *argv[]) {
    if (argc < 2) return 1;

    int N = atoi(argv[1]); // 2. Estilo MATI: Sin variables globales
    long soluciones_totales = 0;
    double timetick = dwalltime();

    #pragma omp parallel for reduction(+:soluciones_totales) schedule(dynamic)
    for (int col = 0; col < N; col++) {
        int tablero_privado[N]; // 3. Estilo MATI: Local en el stack (Más rápido que malloc)
        tablero_privado[0] = col;
        soluciones_totales += resolver(tablero_privado, 1, N);
    }

    printf("N: %d | Soluciones: %ld | Tiempo: %.4f segs\n", N, soluciones_totales, dwalltime() - timetick);
    return 0;
}