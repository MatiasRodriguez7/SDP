#include<stdio.h>
#include<stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>

//VARIABLES COMPARTIDAS
double *A, /* *B,*/ *C;
int N, P;


//FUNCION QUE DEFINE EL COMPORTAMIENTO DE LOS HILOS
void* matmul_thread(void* arg);

// Multiplica dos matrices, A y B, de tamaÃ±o nxn y almacena el resultado en la matriz C
//void matmul(double *A, double *B, double *C, int n);
// Valida el resultado. 0 si ok, sino -1
int validar(int n, double *c, char* fileR);
// Lee una matriz desde archivo de a datos consecutivos y la almacena en el mismo orden en memoria principal 
double* leerMatriz(double *m, int n, char * fullpath);
//Para calcular tiempo
double dwalltime(void);


int main(int argc,char*argv[]){

	// Chequeo de parametros, ahora <6 xq debo pasar P, cantidad de hilos tambien
	if ( (argc < 4) || ((N = atoi(argv[1])) <= 0)){
		printf("\nError en los parametros. Usar: %s N  <ruta y archivo matriz A> <ruta y archivo matriz B> <ruta y archivo matriz resultado> \n", argv[0]);
		exit(1);
	}

	//Guardo o Almaceno el valor total de hilos
	P = atoi(argv[2]);

	// Lee las rutas de los archivos
	char* fileA = argv[3];

	//Aloca memoria para las matrices
	A = (double*)malloc(sizeof(double)*N*N);
	//B = (double*)malloc(sizeof(double)*N*N);
	C = (double*)malloc(sizeof(double)*N*N);

	// Lee las matrices a y b de archivos. Se almacenan en memoria linealmente tal como se encuentran en archivo
	printf("Leyendo matrices...\n");
	A = leerMatriz(A,N,fileA); // Asumimos ordenada en archivo por filas, en memoria la utilizamos por filas

	
	//Realiza la multiplicacion
	printf("Multiplicando matrices...\n");
	double timetick = dwalltime();

		//CREO HILOS MEDIANTE PTHREAD_CREATE
		pthread_t hilos[P];
		int ids[P];
		for(int id = 0; id<P; id++){
			ids[id]=id;
			pthread_create(&hilos[id],NULL,&matmul_thread,(void*)&ids[id]);
		}

		//ESPERO A QUE TODOS LOS HILOS FINALICEN
		for(int i = 0; i < P; i++){
			pthread_join(hilos[i],NULL);
		}

	double workTime = dwalltime() - timetick;

	printf("mm_naive n = %d Tiempo en segundos %f\n", N, workTime);

	// Liberar memoria antes de validar
	free(A);
	//free(B);  
	free(C);
return(0);
}

//-----------------------------------------------------------------

void * matmul_thread(void*arg){

	int id= *(int*)arg; //CASTEO DEL ID
	int i, j, k;

	//Reparto filas dependiendo el hilo y la cantidad de iteraciones
	
	int filas_por_hilo = N/P;  //Cantidad de iteraciones que le toca
	int inicio = id * filas_por_hilo; //Inicio de iteracion
	int fin = (id + 1) * filas_por_hilo; //Fin de iteracion

	/*for(i=inicio;i<fin;i++){
		for(j=0;j<N;j++){
			B[i+N*j] = A[i*N+j];
		}
	}
	*/
	for(i=inicio;i<fin;i++){
		for(j=0;j<N;j++){
			double suma = 0; //Uso variable local para evitar accesos innecesarios a RAM
			for(k=0;k<N;k++){
				suma += A[i*N+k]*A[j+N*k]; //Si B es row-major el acceso a B seria B[k*N + j]
			}
			C[i*N + j] = suma; //Reemplazo el total en dicha posicion, aca no es necesario que sea atomico, puesto que cada bloque for representa un bloque de memoria distinto
		}						//Por ende cada hilo estaria escribiendo en una direccion de memoria diferente a los otros
	} 
	pthread_exit(NULL);
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