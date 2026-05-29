/*****************************************************************************
 * MPI_source.c
 * Programa principal MPI para la paralelización de N-Reinas
 * Sistemas Distribuidos y Paralelos - UNLP 2026
 *
 * Compilación:
 *   mpicc -O2 -o nreinas_par MPI_source.c Pthreads_source.c -lpthread
 *
 * Ejecución (ejemplo 2 nodos, 4 cores cada uno = 8 hilos por proceso):
 *   mpirun -np 2 --hostfile hosts ./nreinas_par 16 8
 *
 * Argumentos:
 *   argv[1]: N (tamaño del tablero, entre 14 y 18)
 *   argv[2]: número de hilos Pthreads por proceso MPI
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "Pthreads_source.h"
#include <sys/time.h>


/*===========================================================================
 * generar_tareas
 *
 * Construye el pool de tareas para este proceso MPI usando dos niveles de
 * fila fijos, lo que produce muchas tareas pequeñas y mejora el balance
 * de carga del pool dinámico de Pthreads.
 *
 * Esta función pertenece a MPI_source.c porque su lógica mezcla:
 *   - Dominio del problema (replicar el bucle de NQueens para generar tareas)
 *   - Distribución MPI     (round-robin por rank para repartir entre nodos)
 * No tiene nada que ver con la creación o sincronización de hilos.
 *
 * Distribución estática entre procesos MPI:
 *   La tarea global número i le corresponde al proceso (i % num_procs).
 *   Con num_procs=2: proceso 0 toma tareas 0,2,4,... y proceso 1 toma 1,3,5,...
 *   Esto intercala tareas pesadas y livianas entre nodos sin comunicación MPI.
 *
 * Dos niveles de fila:
 *   - Bloque BT1: fija reinas en filas 0, 1 y 2 → backtracking arranca en fila 3
 *   - Bloque BT2: fija reinas en filas 0 y 1   → backtracking arranca en fila 2
 *
 * La memoria del arreglo de tareas se reserva aquí (antes de medir tiempo)
 * y el llamador es responsable de liberarla con free() al terminar.
 *=========================================================================*/
static void generar_tareas(int size, int rank, int num_procs, Pool *pool)
{
    /* Constantes del tablero, iguales que en NQueens() secuencial */
    int SIZEE  = size - 1;
    int TOPBIT = 1 << SIZEE;               /* bit de la columna N-1          */
    int MASK   = (1 << size) - 1;          /* N bits a 1                     */

    /* Reserva sobredimensionada: en el peor caso hay O(N²) tareas.
     * Al final se ajusta con realloc al tamaño real. */
    int    capacidad    = size * size * 2;
    Tarea *buf          = (Tarea *)malloc(capacidad * sizeof(Tarea));
    int    total_global = 0;   /* contador global de tareas (antes del filtro) */
    int    total_local  = 0;   /* tareas que le corresponden a este proceso    */

    int bit1, left2, down2, right2, bitmap2, bit_col2;

    /*-----------------------------------------------------------------------
     * BLOQUE BT1: primera reina en (fila 0, col 0), segunda en col BOUND1
     *
     * Replica el bucle:
     *   for (BOUND1=2; BOUND1<SIZEE; BOUND1++) { Backtrack1(2, ...); }
     * de NQueens() secuencial.
     *
     * Segundo nivel: para cada BOUND1, calcula las columnas libres en la
     * fila 2 y genera una tarea por cada columna libre. El backtracking
     * de cada tarea arranca en la fila 3 con las amenazas de las tres
     * primeras reinas ya propagadas.
     *----------------------------------------------------------------------*/
    for (int BOUND1 = 2; BOUND1 < SIZEE; BOUND1++) {

        /* bit1: bitmask de la segunda reina en la columna BOUND1 */
        bit1 = 1 << BOUND1;

        /* Amenazas acumuladas después de colocar reina 0 en col 0
         * y reina 1 en col BOUND1. Son exactamente los valores que
         * NQueens() pasa como (left, down, right) a Backtrack1(2,...). */
        left2  = (2 | bit1) << 1;   /* diagonales izquierdas de filas 0 y 1 */
        down2  =  1 | bit1;         /* columnas 0 y BOUND1 ocupadas          */
        right2 = bit1 >> 1;         /* diagonal derecha de fila 1            */

        /* Columnas libres en fila 2, respetando la restricción de BOUND1:
         * si y=2 < BOUND1, Backtrack1 prohíbe la columna 1 (bit=2).
         * Aplicamos aquí esa misma restricción para no generar tareas
         * inválidas que el backtracking luego descartaría. */
        bitmap2 = MASK & ~(left2 | down2 | right2);
        if (2 < BOUND1) {
            bitmap2 |= 2;   /* activa el bit 1 para luego apagarlo con XOR  */
            bitmap2 ^= 2;   /* fuerza el bit 1 a 0: columna 1 prohibida     */
        }

        /* Segundo nivel: una tarea por cada columna libre en fila 2 */
        while (bitmap2) {
            /* Extrae la columna más a la izquierda disponible */
            bit_col2 = -bitmap2 & bitmap2;
            bitmap2 ^= bit_col2;

            /* Distribución round-robin: esta tarea global le toca al proceso
             * cuyo rank es (total_global % num_procs) */
            if ((total_global % num_procs) == rank) {
                Tarea *t  = &buf[total_local];
                t->tipo   = TIPO_BT1;
                t->board0 = 1;          /* reina fila 0 en col 0 (esquina)  */
                t->board1 = bit1;       /* reina fila 1 en col BOUND1       */
                t->bound1 = BOUND1;
                t->bound2 = 0;          /* no usado en BT1                  */
                t->y_inicio = 3;        /* fila 3 es la primera libre       */

                /* Propagar amenazas de la tercera reina (fila 2, col bit_col2)
                 * para que el backtracking arranque con el estado correcto */
                t->left  = ((left2  | bit_col2) << 1) & MASK;
                t->down  =   down2  | bit_col2;
                t->right = ((right2 | bit_col2) >> 1) & MASK;

                /* BT1 no usa parámetros de simetría */
                t->lastmask = 0;
                t->endbit   = 0;
                t->sidemask = 0;

                total_local++;
            }
            total_global++;
        }
    }

    /*-----------------------------------------------------------------------
     * BLOQUE BT2: primera reina en columna interior BOUND1
     *
     * Replica el bucle:
     *   for (BOUND1=1, BOUND2=N-2; BOUND1<BOUND2; BOUND1++, BOUND2--)
     *       { Backtrack2(1, ...); }
     * de NQueens() secuencial, incluyendo la actualización de LASTMASK
     * y ENDBIT al final de cada iteración.
     *
     * Segundo nivel: para cada BOUND1, calcula las columnas libres en la
     * fila 1 y genera una tarea por cada columna libre. El backtracking
     * de cada tarea arranca en la fila 2 con las amenazas ya propagadas.
     *
     * Los parámetros de simetría (LASTMASK, ENDBIT, SIDEMASK) cambian en
     * cada iteración y se guardan en la tarea para que Backtrack2 y Check()
     * los usen correctamente.
     *----------------------------------------------------------------------*/
    int SIDEMASK = TOPBIT | 1;   /* columnas borde: 0 y N-1                 */
    int LASTMASK = TOPBIT | 1;   /* columnas prohibidas para la última reina */
    int ENDBIT   = TOPBIT >> 1;  /* columna esperada en última fila (180°)   */

    for (int BOUND1 = 1, BOUND2 = size - 2;
         BOUND1 < BOUND2;
         BOUND1++, BOUND2--) {

        /* bit1: bitmask de la primera reina en la columna BOUND1 */
        bit1 = 1 << BOUND1;

        /* Amenazas de la primera reina (a diferencia de BT1, aquí solo
         * hay una reina fija antes de calcular el segundo nivel) */
        int left1  = bit1 << 1;
        int down1  = bit1;
        int right1 = bit1 >> 1;

        /* Columnas libres en fila 1, aplicando restricción de SIDEMASK:
         * si y=1 < BOUND1, Backtrack2 prohíbe las columnas borde.
         * Para BOUND1=1 la condición es falsa (1 < 1 → false), no aplica.
         * Para BOUND1=2 es verdadera (1 < 2 → true), aplica. */
        int bitmap1 = MASK & ~(left1 | down1 | right1);
        if (1 < BOUND1) {
            bitmap1 |= SIDEMASK;
            bitmap1 ^= SIDEMASK;
        }

        /* Segundo nivel: una tarea por cada columna libre en fila 1 */
        while (bitmap1) {
            bit_col2 = -bitmap1 & bitmap1;
            bitmap1 ^= bit_col2;

            if ((total_global % num_procs) == rank) {
                Tarea *t  = &buf[total_local];
                t->tipo   = TIPO_BT2;
                t->board0 = bit1;       /* reina fila 0 en col BOUND1       */
                t->board1 = bit_col2;   /* reina fila 1 en col libre        */
                t->bound1 = BOUND1;
                t->bound2 = BOUND2;
                t->y_inicio = 2;        /* fila 2 es la primera libre       */

                /* Propagar amenazas de las dos primeras reinas */
                t->left  = ((left1  | bit_col2) << 1) & MASK;
                t->down  =   down1  | bit_col2;
                t->right = ((right1 | bit_col2) >> 1) & MASK;

                /* Guardar estado de simetría de esta iteración del bucle:
                 * cada iteración tiene valores distintos de estos tres */
                t->lastmask = LASTMASK;
                t->endbit   = ENDBIT;
                t->sidemask = SIDEMASK;

                total_local++;
            }
            total_global++;
        }

        /* Actualizar LASTMASK y ENDBIT al final de cada iteración,
         * exactamente igual que en NQueens() secuencial */
        LASTMASK |= LASTMASK >> 1 | LASTMASK << 1;
        ENDBIT   >>= 1;
    }

    /* Ajustar el arreglo al tamaño real de tareas de este proceso */
    pool->tareas    = (Tarea *)realloc(buf, total_local * sizeof(Tarea));
    pool->total     = total_local;
    pool->siguiente = 0;
    pthread_mutex_init(&pool->mutex, NULL);
}

/*===========================================================================
 * main
 *=========================================================================*/
int main(int argc, char *argv[])
{
    int rank, num_procs;

    /*-----------------------------------------------------------------------
     * Inicialización MPI
     * MPI_THREAD_FUNNELED: solo el hilo principal llama a MPI.
     * Es suficiente porque las llamadas MPI (Reduce) se hacen antes y
     * después de los hilos Pthreads, nunca durante su ejecución.
     *----------------------------------------------------------------------*/
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    /*-----------------------------------------------------------------------
     * Lectura de argumentos
     *----------------------------------------------------------------------*/
    if (argc < 3) {
        if (rank == 0)
            fprintf(stderr, "Uso: %s <N> <num_hilos>\n", argv[0]);
        MPI_Finalize();
        return 1;
    }

    int size      = atoi(argv[1]);
    int num_hilos = atoi(argv[2]);

    if (size < 4 || size > MAXSIZE) {
        if (rank == 0)
            fprintf(stderr, "N debe estar entre 4 y %d\n", MAXSIZE);
        MPI_Finalize();
        return 1;
    }


    Pool pool;
    //alocacion de hilos y argumentos de hilos para lanzar_hilos
    pthread_t *hilos = (pthread_t *)malloc(num_hilos * sizeof(pthread_t));
    long int *count8_local = (long int *)malloc(num_hilos * sizeof(long int));
    long int *count4_local = (long int *)malloc(num_hilos * sizeof(long int));
    long int *count2_local = (long int *)malloc(num_hilos * sizeof(long int));
    double *tiempo = (double *)malloc(num_hilos * sizeof(double));//vetor para calculo de tiempo de hilos
    long int *idx = (long int *)malloc(num_hilos * sizeof(long int));


    /*-----------------------------------------------------------------------
     * Inicio de la medición de tiempo (solo cómputo + comunicación)
     *
     * El tiempo arranca DESPUÉS de la inicialización del pool y
     * ANTES de lanzar los hilos, tal como pide el enunciado:
     * NO incluir alocación/inicialización de estructuras.
     *----------------------------------------------------------------------*/
    MPI_Barrier(MPI_COMM_WORLD);
    double t_ini = dwalltime();

        /*-----------------------------------------------------------------------
     * Generación del pool de tareas (distribución estática entre nodos)
     *
     * Cada proceso genera SOLO las tareas que le corresponden según la
     * distribución ciclica: tarea i → proceso (i % num_procs).
     * No hay comunicación MPI en esta etapa.
     *----------------------------------------------------------------------*/
    generar_tareas(size, rank, num_procs, &pool);

    /*-----------------------------------------------------------------------
     * Lanzamiento de hilos Pthreads
     *
     * Todos los hilos del proceso compiten por tareas del pool local.
     *----------------------------------------------------------------------*/

    lanzar_hilos(size, num_hilos, &pool, hilos, tiempo, idx,
                 count8_local, count4_local, count2_local);

    /*-----------------------------------------------------------------------
     * Reducción MPI: suma de contadores de todos los procesos hacia rank 0
     *
     * Cada proceso envía sus contadores locales. El proceso 0 recibe la
     * suma global. Los demás procesos reciben el resultado pero lo ignoran
     * (MPI_Reduce con root=0 es suficiente; no se necesita MPI_Allreduce).
     *----------------------------------------------------------------------*/
    long int count8_total = 0;
    long int count4_total = 0;
    long int count2_total = 0;

    long int count8_nodo = 0;
    long int count4_nodo = 0;
    long int count2_nodo = 0;

    for (int i = 0; i < num_hilos; i++) {
        count8_nodo += count8_local[i];
        count4_nodo += count4_local[i];
        count2_nodo += count2_local[i];
    }

    MPI_Reduce(&count8_nodo, &count8_total, 1, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&count4_nodo, &count4_total, 1, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&count2_nodo, &count2_total, 1, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

    /*-----------------------------------------------------------------------
     * Fin de la medición de tiempo
     *----------------------------------------------------------------------*/
    double t_fin = dwalltime();

    /*-----------------------------------------------------------------------
     * Resultado final (solo el proceso 0 calcula e imprime)
     *----------------------------------------------------------------------*/
    if (rank == 0) {
        long int unique = count8_total + count4_total + count2_total;
        long int total  = count8_total * 8 + count4_total * 4 + count2_total * 2;

        printf("N=%d  Hilos por nodo=%d  Procesos MPI=%d\n",size, num_hilos, num_procs);
        printf("Soluciones únicas : %ld\n", unique);
        printf("Soluciones totales: %ld\n", total);
        printf("Tiempo de ejecución: %.6f segundos\n", t_fin - t_ini);
    }
    double promedio = 0;
    for(int i = 0; i < num_hilos; i++){
        printf("Nodo %d: Tiempo del hilo %d: %.6f segundos\n", rank, i, tiempo[i]);
        //calcular promiedo de tiempo por nodo
        promedio += tiempo[i];
    }

    promedio /= num_hilos;
    printf("Nodo %d: Promedio de tiempo de los hilos: %.6f segundos\n", rank, promedio);

    /*-----------------------------------------------------------------------
     * Liberación de recursos
     *----------------------------------------------------------------------*/
    /*-----------------------------------------------------------------------
     * Liberación de recursos
     *----------------------------------------------------------------------*/
    free(pool.tareas);
    free(hilos);
    free(count8_local);
    free(count4_local);
    free(count2_local);
    free(tiempo);
    free(idx);
    pthread_mutex_destroy(&pool.mutex);
    MPI_Finalize();
    return 0;
}
