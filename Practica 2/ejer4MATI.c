#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <omp.h>
#include <sys/time.h>

// Función para medir el tiempo (asumiendo que usas la misma de los ejercicios anteriores)
double dwalltime(void){
    double sec;
    struct timeval tv;
    gettimeofday(&tv,NULL);
    sec = tv.tv_sec + tv.tv_usec/1000000.0;
    return sec;
}

// Verifica si es seguro colocar una reina en (fila, col)
bool es_seguro(int tablero[], int fila, int col) {
    for (int i = 0; i < fila; i++) {
        // Verifica si hay una reina en la misma columna o en las diagonales
        if (tablero[i] == col || abs(tablero[i] - col) == abs(i - fila)) {
            return false;
        }
    }
    return true;
}

// Función recursiva SECUENCIAL (cada hilo ejecuta esto por su cuenta)
void resolver_nreinas_recursivo(int tablero[], int fila, int N, int *soluciones_locales) {
    // Caso base: Si llegamos a la fila N, encontramos una solución válida
    if (fila == N) {
        (*soluciones_locales)++;
        return;
    }

    // Intentamos colocar la reina en todas las columnas de la fila actual
    for (int col = 0; col < N; col++) {
        if (es_seguro(tablero, fila, col)) {
            tablero[fila] = col; // Colocamos la reina
            
            // Llamada recursiva para la siguiente fila
            resolver_nreinas_recursivo(tablero, fila + 1, N, soluciones_locales);
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Uso: %s <N (tamaño del tablero)>\n", argv[0]);
        return 1;
    }

    int N = atoi(argv[1]);
    int soluciones_totales = 0;

    double timetick = dwalltime();

    // PARALELIZACIÓN DEL PRIMER NIVEL (Raíz del árbol)
    // Usamos schedule(dynamic, 1) porque cada columna inicial tomará un tiempo impredecible.
    // Usamos reduction(+:soluciones_totales) para sumar de forma segura sin usar un #pragma omp critical
    #pragma omp parallel for schedule(dynamic, 1) reduction(+:soluciones_totales)
    for (int col_inicial = 0; col_inicial < N; col_inicial++) {
        
        // ¡DETALLE CRÍTICO! Cada hilo NECESITA su propio tablero.
        // Si usáramos un tablero global/compartido, los hilos se sobreescribirían
        // las posiciones de las reinas constantemente arruinando el backtracking.
        int tablero_local[N]; 
        int soluciones_locales = 0;

        // Fijamos la posición de la primera reina (fila 0) para este hilo
        tablero_local[0] = col_inicial;

        // A partir de la fila 1, el hilo explora su sub-árbol de forma secuencial
        resolver_nreinas_recursivo(tablero_local, 1, N, &soluciones_locales);

        // Sumamos lo que encontró este hilo al total (OpenMP lo maneja por el reduction)
        soluciones_totales += soluciones_locales;
    }

    double tiempo_total = dwalltime() - timetick;

    printf("N: %d\n", N);
    printf("Soluciones encontradas: %d\n", soluciones_totales);
    printf("Tiempo de ejecución: %f segundos\n", tiempo_total);

    return 0;
}