#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h> 
#include <mpi.h>

// Modificamos la firma para que reciba los buffers locales y la cantidad de iteraciones (filas)
void fA(int, int, double*, double*, double*); 
// Valida el resultado. 0 si ok, sino -1
int validar(int n, double *c, char* fileR);
double dwalltime(void);
double* leerMatriz(double *m, int n, char * fullpath);

int main(int argc, char* argv[]){
    // Verificacion parametros
    double ini, fin;
    if ( (argc < 4)){
        printf("\nError en los parametros. Usar: %s N  <ruta y archivo matriz A> <ruta y archivo matriz B> <ruta y archivo matriz resultado> \n", argv[0]);
        exit(1);
    }
    
    int id, nProcs;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &nProcs);

    int N = atoi(argv[1]);
    char* fileA = argv[2];
    char* fileB = argv[3];
    char* fileR = argv[4];
    
    int iteraciones = N / nProcs; // Lo que antes llamábamos filas_por_proceso

    // Punteros para la matriz completa (Solo el Proceso 0 las va a instanciar enteras)
    double *A;
    double *C;
    // Todos los procesos necesitan conocer B entera para multiplicar
    double *B = (double*)malloc(sizeof(double)*N*N);

    // Buffers locales: cada proceso solo aloja la memoria para SUS filas
    double *mi_A = (double*)malloc(sizeof(double) * iteraciones * N);
    double *mi_C = (double*)calloc(iteraciones * N, sizeof(double));

    // 1. El Proceso 0 lee los datos
    if (id == 0) {
        A = (double*)malloc(sizeof(double)*N*N);
        C = (double*)malloc(sizeof(double)*N*N);
        
        A = leerMatriz(A, N, fileA); 
        B = leerMatriz(B, N, fileB); 
        ini =  dwalltime();
    }

    // 2. El Proceso 0 envía la matriz B completa a TODOS los procesos
    MPI_Bcast(B, N * N, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // 3. El Proceso 0 reparte las filas de A entre todos los procesos
    MPI_Scatter(A, iteraciones * N, MPI_DOUBLE, 
                mi_A, iteraciones * N, MPI_DOUBLE, 
                0, MPI_COMM_WORLD);

    // 4. COMPUTO PARALELO
    fA(N, iteraciones, mi_A, B, mi_C);

    // 5. El Proceso 0 recolecta los pedazos calculados (mi_C) y arma la matriz final (C)
    MPI_Gather(mi_C, iteraciones * N, MPI_DOUBLE, C, iteraciones * N, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // 6. Limpieza
    free(B);
    free(mi_A);
    free(mi_C);
    
    if (id == 0) {
        free(A);

        // Valida
	    printf("Validando...\n");
	    if (validar(N,C,fileR) == 0)
	    	printf("Resultado correcto.\n");
    	else
    		printf("Error.\n");
        fin =  dwalltime() - ini;
        printf("mm_naive n = %d Tiempo en segundos %f\n", N, fin);
        free(C);
    }



    MPI_Finalize();
    return 0;
}

int validar(int n, double *c, char* fileR){
    int validacion = 0;
    double* r = (double *) malloc(n*n*sizeof(double));

	leerMatriz(r,n,fileR);

    if (memcmp(r, c, n*n*sizeof(double)) != 0) {      		
    	validacion = -1;
    }
	
	free(r);
	return validacion;
}


void fA(int N, int iteraciones, double * mi_A, double * B, double * mi_C){
    // Cada proceso trabaja solo sobre "mi_A" y guarda en "mi_C"
    // Orden optimizado por bloques/caché: i-k-j
    for (int i = 0; i < iteraciones; i++) {
        for (int k = 0; k < N; k++) {
            for (int j = 0; j < N; j++) {
                mi_C[i * N + j] += mi_A[i * N + k] * B[k * N + j];
            }
        }
    }
}

double* leerMatriz(double *m, int n, char * fullpath){
    int i, j;
    double val;
    FILE* archivo;

    archivo = fopen(fullpath, "rb");
    if (!archivo) {
        perror("Error al abrir el archivo\n");
        return NULL;
    }

    fread(m, sizeof(double), n * n, archivo);
    fclose(archivo);

    return m;
}

//MEDIR TIEMPOS
double dwalltime(void){
        double sec;
        struct timeval tv;

        gettimeofday(&tv,NULL);
        sec = tv.tv_sec + tv.tv_usec/1000000.0;
        return sec;
}