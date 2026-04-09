#include<stdio.h>
#include<stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h> 
#include <pthread.h>

void * fHilos(void * arg);//Comportamiento hilos
double dwalltime(void);
// Lee una matriz desde archivo de a datos consecutivos y la almacena en el mismo orden en memoria principal 
double* leerMatriz(double *m, int n, char * fullpath);

//VARIABLES COMPARTIDAS
int N,P;
double *A;
double *mx;
double *mn;
double *suma;

int main(int argc, char* argv[]){
    //verificacion parametros
    if ( (argc < 3) || ((N = atoi(argv[1])) <= 0)){
		printf("\nError en los parametros. Usar: %s N  <ruta y archivo matriz A> <ruta y archivo matriz B> <ruta y archivo matriz resultado> \n", argv[0]);
		exit(1);
	}

    //Reservo memoria para la matriz
    char* fileA = argv[3];
    A = (double*)malloc(sizeof(double)*N*N);
    A = leerMatriz(A,N,fileA); // Asumimos ordenada en archivo por filas, en memoria la utilizamos por filas

    P = atoi(argv[2]);
    //reservo memoria para vectores de resultados parciales
    mx = (double *)malloc(P * sizeof(double));
    mn = (double *)malloc(P * sizeof(double));
    suma = (double *)malloc(P * sizeof(double)); 

    //resultados finales
    double max = -1;
    double min = 99999;
    double sum = 0;

    //definicion hilos
    pthread_t hilos[P];
    int ids[P];

    double inicio = dwalltime();
    //asigno id y creo hilo(inicia a correr)
    for(int i = 0; i < P; i++){
        ids[i]=i;
        pthread_create(&hilos[i], NULL,&fHilos,(void*)&ids[i]);
    }

    //espera por la finalizacion de todos ellos
    for(int i = 0; i < P; i++){
        pthread_join(hilos[i],NULL);
    }

    //barrera implicita por el join de arriba

    //calculo de min, max, prom totales
    for(int i = 0; i<P; i++){
        if(min>mn[i])
            min=mn[i];
        if(max<mx[i])
            max=mx[i];
        sum+=suma[i];
    }
    sum=sum/(N*N);
    double fin = dwalltime()-inicio;

    printf("tiempo trabajo n = %d Tiempo en segundos %f\n", N, fin);
    printf("max %.17f\n",max);
    printf("prom %.17f\n",sum);
    printf("min %.17f\n",min);

    free(A);
    free(mn);
    free(mx);
    free(suma);
    return 0;
}

void * fHilos(void * arg){
    int id = *(int*)arg;

    int iteraciones = (N*N)/P;
    int inicio = id * iteraciones;
    int fin = (id + 1) * iteraciones;
    double max = -1;
    double sum = 0;
    double min = 999999;

    for(int i=inicio;i<iteraciones;i++){
        if(max<A[i]){
            max=A[i];
        }
        if(min>A[i]){
            min=A[i];
        }
        sum+=A[i];
    }
    //guarda sus resultados parciales
    suma[id]=sum;
    mx[id]=max;
    mn[id]=min;

    pthread_exit(NULL);
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