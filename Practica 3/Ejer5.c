#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <mpi.h>

#define TRUE 1
#define FALSE 0

// Funciones originales
void mezcla(int* arregloA, int NA, int* arregloB, int NB, int* arregloDeSalida);
void ordenacion(int** arreglo, int N, int** arregloAuxiliar);
void intercambio(int** p1, int** p2);
int* inicializarArreglo(int* arreglo, int N);
int validar(int* arreglo, int N);
double dwalltime(void);

int main(int argc, char* argv[]) {
    int id, nProcs, exp;
    double ini, fin;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &nProcs);

    // Chequeo de parámetros
    if (argc != 2) {
        if (id == 0) printf("Uso: mpirun -np P %s <exponente>\n", argv[0]);
        MPI_Finalize();
        return 1;
    }

    exp = atoi(argv[1]);
    if (exp < 0 || exp > 30) {
        if (id == 0) printf("El exponente debe estar entre 0 y 30\n");
        MPI_Finalize();
        return 1;
    }

    int N = 1 << exp; // Calcula N = 2^exp
    
    // Validamos que nProcs sea potencia de 2 para la reducción en árbol perfecta
    if ((nProcs & (nProcs - 1)) != 0) {
        if (id == 0) printf("Error: La cantidad de procesos (P) debe ser potencia de 2 (Ej: 2, 4, 8).\n");
        MPI_Finalize();
        return 1;
    }

    int local_size = N / nProcs;

    int* arreglo_global = NULL;
    
    if (id == 0) {
        arreglo_global = (int*)malloc(sizeof(int) * N);
        arreglo_global = inicializarArreglo(arreglo_global, N);
        printf("Ordenando %d elementos con %d procesos (Merge Sort en Arbol)...\n", N, nProcs);
        ini = dwalltime();
    }

    // 1. Buffers locales para la primera etapa
    int* arreglo_local = (int*)malloc(sizeof(int) * local_size);
    int* arreglo_aux = (int*)malloc(sizeof(int) * local_size);

    // 2. El Master reparte los pedazos desordenados
    MPI_Scatter(arreglo_global, local_size, MPI_INT, 
                arreglo_local, local_size, MPI_INT, 
                0, MPI_COMM_WORLD);

    // 3. Cada proceso ordena su fragmento de forma local (Secuencialmente)
    ordenacion(&arreglo_local, local_size, &arreglo_aux);
    
    // Como ordenacion intercambia punteros, arreglo_local ahora apunta al buffer ordenado. 
    // Podemos liberar arreglo_aux porque ya no lo necesitamos.
    free(arreglo_aux);

    // 4. FUSIÓN EN ÁRBOL (Tree Merge Reduction)
    int paso = 1;
    while (paso < nProcs) {
        if (id % (2 * paso) == 0) {
            // --- SOY RECEPTOR ---
            int origen = id + paso;
            if (origen < nProcs) {
                // El tamaño que voy a recibir es igual al que ya tengo
                int recv_size = local_size; 
                int* recv_buffer = (int*)malloc(sizeof(int) * recv_size);
                
                MPI_Recv(recv_buffer, recv_size, MPI_INT, origen, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                // Creo un buffer del doble de tamaño para alojar la mezcla
                int* merged_buffer = (int*)malloc(sizeof(int) * (local_size + recv_size));
                
                // Uso tu función original para mezclar mi parte con la recibida
                mezcla(arreglo_local, local_size, recv_buffer, recv_size, merged_buffer);

                // Libero los buffers viejos y actualizo los punteros
                free(arreglo_local);
                free(recv_buffer);
                arreglo_local = merged_buffer;
                local_size += recv_size; // Mi tamaño se duplicó
            }
        } else {
            // --- SOY EMISOR ---
            int destino = id - paso;
            MPI_Send(arreglo_local, local_size, MPI_INT, destino, 0, MPI_COMM_WORLD);
            break; // Una vez que envié mis datos, mi participación en el árbol termina
        }
        paso *= 2;
    }

    // 5. El Master (id == 0) se quedó con el arreglo completo ordenado en su 'arreglo_local'
    if (id == 0) {
        fin = dwalltime() - ini;
        printf("Tiempo paralelo en segundos: %f \n", fin);
        
        printf("Validando...\n");
        if (validar(arreglo_local, N))
            printf("--ORDENADO OK--\n");
        else
            printf("--ERROR--\n");

        free(arreglo_global);
    }

    // Liberamos la memoria del último buffer que le quedó a cada proceso (sea el Master o un Worker antes del break)
    free(arreglo_local);

    MPI_Finalize();
    return 0;
}

// ---------------------------------------------------------
// FUNCIONES ORIGINALES INTACTAS
// ---------------------------------------------------------

void mezcla(int* arregloA, int NA, int* arregloB, int NB, int* arregloDeSalida){
    int indiceA = 0;
    int indiceB = 0;
    int indiceDeSalida;
    int limiteIndiceDeSalida = NA + NB;
 
    for(indiceDeSalida=0; indiceDeSalida < limiteIndiceDeSalida; indiceDeSalida++){
        if( (indiceB == NB) || ( (indiceA < NA) && (arregloA[indiceA] <= arregloB[indiceB]) ) ){
            arregloDeSalida[indiceDeSalida] = arregloA[indiceA];
            indiceA++;
        }else if( (indiceA == NA) || ( (indiceB < NB) && (arregloA[indiceA] > arregloB[indiceB]) ) ){
            arregloDeSalida[indiceDeSalida] = arregloB[indiceB];
            indiceB++;    
        }        
    }
}

void ordenacion(int** arreglo, int N, int** arregloAuxiliar){
    int longitudDeLaParte;
    int posicionDeLaParte;
    int mitad = N/2;
    int* arregloA;
    int* arregloB;
    int* arregloDeSalida;
    
    for(longitudDeLaParte=1; longitudDeLaParte<=mitad; longitudDeLaParte*=2){
        for(posicionDeLaParte=0; posicionDeLaParte<N; posicionDeLaParte+=2*longitudDeLaParte){
            arregloA = *arreglo + posicionDeLaParte;
            arregloB = arregloA + longitudDeLaParte;
            arregloDeSalida = *arregloAuxiliar + posicionDeLaParte;
            mezcla(arregloA, longitudDeLaParte, arregloB, longitudDeLaParte, arregloDeSalida);    
        }                
        intercambio(arreglo, arregloAuxiliar);            
    }    
}

void intercambio(int** p1, int** p2){
    int* aux = *p1;    
    *p1 = *p2;
    *p2 = aux;
}

int* inicializarArreglo(int* arreglo, int N){
    srand(time(NULL));
    for (int i = 0; i < N; i++) {
        arreglo[i] = rand() % 100;
    }
    return arreglo;
}

int validar(int* arreglo, int N){
    int ordenado = TRUE;
    int i = 1;
    while (i < N && ordenado) {
        ordenado = (arreglo[i-1] <= arreglo[i]);
        i++;
    }
    return ordenado;
}

double dwalltime(void){
    double sec;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    sec = tv.tv_sec + tv.tv_usec/1000000.0;
    return sec;
}