#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h> 
#include <mpi.h>


void matmulCol(int iteraciones, double *mi_A, double *A_col, double *mi_C, int N);

//Toma la matriz A ordenada por filas y la ordena por columnas en A_col
void ordenarCol(double *A, double *A_col, int n);

// Lee una matriz desde archivo de a datos consecutivos y la almacena en el mismo orden en memoria principal 
double* leerMatriz(double *m, int n, char * fullpath);
//Para calcular tiempo
double dwalltime(void);

int main(int argc,char*argv[]){
	int N;
	double ini, fin;
	// Chequeo de parametros
	if ( (argc < 2)){
		printf("\nError en los parametros. Usar: %s N  <ruta y archivo matriz A> <ruta y archivo matriz B> <ruta y archivo matriz resultado> \n", argv[0]);
		exit(1);
	}

    int id, nProcs;
	MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &nProcs);

    N = atoi(argv[1]);//tamanio matriz
    char* fileA=argv[2];

	double* A;
	double* C;
	double* A_col = (double*)malloc(sizeof(double)*N*N);//MATRIZ A TRASPUESTA

	//divide las N filas entre los nProcs procesos;
	int iteraciones = N/nProcs;
	
	// Buffers locales: cada proceso solo aloja la memoria para SUS filas
    double *mi_A = (double*)malloc(sizeof(double) * iteraciones * N);
    double *mi_C = (double*)calloc(iteraciones * N, sizeof(double));

	//Si soy el proceso 0(para hacerlo una unica vez) alloco memoria
	if(id==0){
		//Aloca memoria para las matrices
		A = (double*)malloc(sizeof(double)*N*N);	//MATRIZ A ORIGINAL
		C = (double*)malloc(sizeof(double)*N*N);	//MATRIZ RESULTADO
		printf("Leyendo matrices...\n");
		A = leerMatriz(A,N,fileA); 
		ini = dwalltime();
		ordenarCol(A, A_col, N);//Analizamos paralelizarlo, pero bajo un esquema master-worker, el master se sigue llevando gran parte del trabajo con el adicional de agregar overhead por comunicacion
	}

	// 2. El Proceso 0 envía la matriz A_col completa a TODOS los procesos
	MPI_Bcast(A_col, N*N, MPI_DOUBLE, 0, MPI_COMM_WORLD);

	// 3. El Proceso 0 reparte las filas de A entre todos los procesos
    MPI_Scatter(A, iteraciones * N, MPI_DOUBLE, 
                mi_A, iteraciones * N, MPI_DOUBLE, 
                0, MPI_COMM_WORLD);

    // 4. COMPUTO PARALELO
    matmulCol(iteraciones, mi_A, A_col, mi_C, N);
	
    // 5. El Proceso 0 recolecta los pedazos calculados (mi_C) y arma la matriz final (C)
    MPI_Gather(mi_C, iteraciones * N, MPI_DOUBLE, C, iteraciones * N, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	
	free(mi_A);
	free(mi_C);
	free(A_col);

	if(id==0){
		fin = dwalltime() - ini;
		free(A);
		free(C);
		printf("AxAcol n = %d Tiempo en segundos %f\n", N, fin);
	}
	MPI_Finalize();
	return(0);
}

//-----------------------------------------------------------------

void ordenarCol(double *A, double *A_col, int n){
	int i, j;
	for(i=0;i<n;i++){
		for(j=0;j<n;j++){
			A_col[i+n*j] = A[i*n+j];
		}
	}
}

//FUNCION QUE EJECUTA CADA PROCESO
void matmulCol(int iteraciones, double *mi_A, double *A_col, double *mi_C, int N){
int i, j, k;
	for(i=0;i<iteraciones;i++){
		for(j=0;j<N;j++){
			mi_C[i*N+j] = 0;//Acceso por filas
			for(k=0;k<N;k++){
				mi_C[i*N+j] += mi_A[i*N+k] * A_col[(j*N)+k];
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

double dwalltime(void){
        double sec;
        struct timeval tv;

        gettimeofday(&tv,NULL);
        sec = tv.tv_sec + tv.tv_usec/1000000.0;
        return sec;
}
