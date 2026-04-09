#include<stdio.h>
#include<stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h> 
#include <omp.h>
int N; // Tamaño del tablero

// Función de validación: ¿Es seguro poner una reina en tablero[fila] = columna?
int es_seguro(int *tablero, int fila, int col) {
    for (int i = 0; i < fila; i++) {
        // Misma columna o diagonales
        if (tablero[i] == col || abs(tablero[i] - col) == abs(i - fila)) {
            return 0;
        }
    }
    return 1;
}

// Algoritmo de Backtracking recursivo
long resolver(int *tablero, int fila) {
    if (fila == N) return 1; // Se encontró una solución completa

    long soluciones = 0;
    for (int col = 0; col < N; col++) {
        if (es_seguro(tablero, fila, col)) {
            tablero[fila] = col;
            soluciones += resolver(tablero, fila + 1);
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
    if (argc < 2) {
        printf("Uso: %s N\n", argv[0]);
        return 1;
    }

    N = atoi(argv[1]);
    long soluciones_totales = 0;
    double timetick = dwalltime();

    // Paralelizamos la colocación de la reina en la primera fila (fila 0)
    // Usamos schedule(dynamic) para corregir el desbalance de carga
    #pragma omp parallel for reduction(+:soluciones_totales) schedule(dynamic)
    for (int col = 0; col < N; col++) {
        // Cada hilo necesita su propio tablero privado para su subárbol
        int *tablero_privado = (int *)malloc(N * sizeof(int));

        tablero_privado[0] = col;
        soluciones_totales += resolver(tablero_privado, 1);

        free(tablero_privado);
    }

    double tiempo = dwalltime() - timetick;
    printf("N: %d | Soluciones: %ld | Tiempo: %.4f segs\n", N, soluciones_totales, tiempo);

    return 0;
}