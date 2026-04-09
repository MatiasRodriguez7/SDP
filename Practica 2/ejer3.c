#include<stdio.h>
#include<stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h> 
#include <omp.h>

//PROTOTIPOS FUNCIONES
double dwalltime(void);
// Lee una matriz desde archivo de a datos consecutivos y la almacena en el mismo orden en memoria principal 
double* leerMatriz(double *m, int n, char * fullpath);

//VARIABLES COMPARTIDAS
int N,tid,i,j;
double timetick,temp;
double *A;

int main(int argc, char* argv[]){
    if ( (argc < 2) || ((N = atoi(argv[1])) <= 0)){
		printf("\nError en los parametros. Usar: %s N  <ruta y archivo matriz A> <ruta y archivo matriz B> <ruta y archivo matriz resultado> \n", argv[0]);
		exit(1);
	}

    //Reservo memoria para la matriz
    char* fileA = argv[2];
    A = (double*)malloc(sizeof(double)*N*N);
    A = leerMatriz(A,N,fileA); // Asumimos ordenada en archivo por filas, en memoria la utilizamos por filas

    #pragma omp parallel default(none) private(i,j,temp,timetick,tid) shared(A,N) 
    {
        tid= omp_get_thread_num();
        timetick = dwalltime();
        #pragma omp for private(i,j,temp) schedule(guided,256) nowait
            for(i=0;i<N;i++){
                for(j=i+1;j<N;j++){
                    temp = A[i*N+j];
                    A[i*N+j]= A[j*N+i];
                    A[j*N+i]= temp;
                }
            }
        printf("Tiempo para el thread %d: %f segs\n", tid,dwalltime() - timetick);
    }
    free(A);
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