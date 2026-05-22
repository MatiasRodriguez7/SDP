#include <mpi.h>
#include "Pthreads_source.h"
#include<stdio.h>
#include<stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>

int validar(int n, double *c, char* fileR);
// Lee una matriz desde archivo de a datos consecutivos y la almacena en el mismo orden en memoria principal 
double* leerMatriz(double *m, int n, char * fullpath);
//Para calcular tiempo
double dwalltime(void);

int main(int argc, char* argv[]){
    int rank;
    int provided;//variable que se usa para recibir el nivel de acceso a MPI que nos dan
    int nProcs;
    double ini, fin;
    par_t sh_p;
    // Chequeo de parametros
    if ( (argc < 5)){
        printf("\nError en los parametros. Usar: %s N  <ruta y archivo matriz A> <ruta y archivo matriz B> <ruta y archivo matriz resultado> \n", argv[0]);
        exit(1);
    }

    //MPI_THREAD_FUNNELED--> deja que solo el hilo que llama la funcion acceda a MPI
    MPI_Init_thread(&argc, &argv,MPI_THREAD_FUNNELED,&provided);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    MPI_Comm_size(MPI_COMM_WORLD,&nProcs);
    
    int N = atoi(argv[1]);
    char* fileA = argv[2];
    char* fileB = argv[3];
    char* fileR = argv[4];
    int T = atoi(argv[5]);
    
    int iteraciones = N / nProcs; // Lo que antes llamábamos filas_por_proceso

    // Punteros para la matriz completa (Solo el Proceso 0 las va a instanciar enteras)
    double *A;
    double *C;
    double *R;
    // Todos los procesos necesitan conocer B entera para multiplicar
    double *B = (double*)malloc(sizeof(double)*N*N);

    // Buffers locales: cada proceso solo aloja la memoria para SUS filas
    double *mi_A = (double*)malloc(sizeof(double) * iteraciones * N);
    double *mi_C = (double*)calloc(iteraciones * N, sizeof(double));

    // 1. El Proceso 0 lee los datos
    if (rank == 0) {
        A = (double*)malloc(sizeof(double)*N*N);
        C = (double*)malloc(sizeof(double)*N*N);
        R = (double*)malloc(sizeof(double)*N*N);
        A = leerMatriz(A, N, fileA); 
        B = leerMatriz(B, N, fileB); 
        R = leerMatriz(R, N, fileR); 
        ini=dwalltime();
    }
    
    sh_p.mi_A=mi_A;
    sh_p.B=B;
    sh_p.mi_C=mi_C;
    sh_p.T=T;
    sh_p.N=N;
    sh_p.iteraciones=iteraciones;

    // 2. El Proceso 0 envía la matriz B completa a TODOS los procesos
    MPI_Bcast(B, N * N, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // 3. El Proceso 0 reparte las filas de A entre todos los procesos
    MPI_Scatter(A, iteraciones * N, MPI_DOUBLE, 
                mi_A, iteraciones * N, MPI_DOUBLE, 
                0, MPI_COMM_WORLD);    

    pthreads_function(sh_p);

    // 5. El Proceso 0 recolecta los pedazos calculados (mi_C) y arma la matriz final (C)
    MPI_Gather(mi_C, iteraciones * N, MPI_DOUBLE, C, iteraciones * N, MPI_DOUBLE, 0, MPI_COMM_WORLD);    
    if(rank==0){
        fin=dwalltime()-ini;
        printf("mm_naive n = %d Tiempo en segundos %f\n", N, fin);
        free(A); // <- ME HABIA OLVIDADO ESTE!

        // Valida solo el proceso 0
        printf("Validando...\n");
        if (validar(N,C,fileR) == 0)
            printf("Resultado correcto.\n");
        else
            printf("Error.\n");
            
        free(R);
        free(C);
    }
    
    // Todos los procesos liberan su propia memoria
    free(B); 
    free(mi_A);
    free(mi_C);

    MPI_Finalize();
    return(0);
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

double* leerMatriz(double *m, int n, char * fullpath){
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

double dwalltime(void){
    double sec;
    struct timeval tv;

    gettimeofday(&tv,NULL);
    sec = tv.tv_sec + tv.tv_usec/1000000.0;
    return sec;
}