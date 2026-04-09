#include<stdio.h>
#include<stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h> 
#include <omp.h>

double dwalltime(void);
// Lee una matriz desde archivo de a datos consecutivos y la almacena en el mismo orden en memoria principal 
double* leerMatriz(double *m, int n, char * fullpath);
//VARIABLES COMPARTIDAS
int N;
double *A;

int main(int argc, char* argv[]){
//verificacion parametros
    if ( (argc < 2) || ((N = atoi(argv[1])) <= 0)){
		printf("\nError en los parametros. Usar: %s N  <ruta y archivo matriz A> <ruta y archivo matriz B> <ruta y archivo matriz resultado> \n", argv[0]);
		exit(1);
	}

//Reservo memoria para la matriz
    char* fileA = argv[2];
    A = (double*)malloc(sizeof(double)*N*N);
    A = leerMatriz(A,N,fileA); // Asumimos ordenada en archivo por filas, en memoria la utilizamos por filas

    double mx = -1;
    double mn = 99999;
    double suma = 0;

    //Comienzo a medir tiempo
    double inicio=dwalltime();

    //Paralelizo ambos for
    //A[1]=1000000;
    #pragma omp parallel for reduction(max:mx) reduction(min:mn) reduction(+:suma)
    for(int i=0;i<N*N;i++){
        if(mx<A[i]){
            mx=A[i];
        }
        if(mn>A[i]){
            mn=A[i];
        }
        suma+=A[i];
    }//Barrera implicita

    //Vuelvo a secuencial
        printf("suma %.17f\n",suma);

    suma=suma/(N*N); //suma ahora es promedio

    double fin=dwalltime()-inicio;
    printf("max %.17f\n",mx);
    printf("prom %.17f\n",suma);
    printf("min %.17f\n",mn);

    printf("tiempo trabajo n = %d Tiempo en segundos %f\n", N, fin);

    free(A);
    return 0;
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