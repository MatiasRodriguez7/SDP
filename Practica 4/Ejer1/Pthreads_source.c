#include <pthread.h>
#include "Pthreads_source.h"
par_t sp;
void* f(void *arg){
    int tid = *(int*)arg;
    int i, j, k;
	//Reparto filas dependiendo el hilo y la cantidad de iteraciones
	int filas_por_hilo = sp.iteraciones/sp.T;  //Cantidad de iteraciones que le toca
	int inicio = tid * filas_por_hilo; //Inicio de iteracion
	int fin = (tid + 1) * filas_por_hilo; //Fin de iteracion
	for(i=inicio;i<fin;i++){
		for(j=0;j<sp.N;j++){
			double suma = 0; //Uso variable local para evitar accesos innecesarios a RAM
			for(k=0;k<sp.N;k++){
                //suma += A[i*N+k]*B[j+N*k];
				suma += sp.mi_A[i*sp.N+k]*sp.B[j*sp.N+k]; //Si B es row-major el acceso a B seria B[k*N + j]
			}
			sp.mi_C[i*sp.N + j] = suma; //Reemplazo el total en dicha posicion, aca no es necesario que sea atomico, puesto que cada bloque for representa un bloque de memoria distinto
		}						//Por ende cada hilo estaria escribiendo en una direccion de memoria diferente a los otros
	} 
	pthread_exit(NULL);
}

void pthreads_function(par_t sh_p){
    sp=sh_p;
    pthread_t h[sp.T];
    int tids[sp.T];
    for(int tid=0;tid<sp.T;tid++){
        tids[tid] = tid;
        pthread_create(&h[tid], NULL, &f, &tids[tid]);
    }
    for(int tid=0;tid<sp.T;tid++) // Sincronizan
        pthread_join(h[tid],NULL);
}
