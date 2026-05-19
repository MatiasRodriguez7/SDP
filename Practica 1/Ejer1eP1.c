#include<stdio.h>
#include<stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>

typedef enum {
	ORDENXFILAS,
	ORDENXCOLUMNAS
} TOrden;

// Multiplica dos matrices, A y B, de tamaño nxn y almacena el resultado en la matriz C
void matmul(double *A, double *B, double *C, int n);

// Multiplica dos matrices, A y B, de tamaño nxn y almacena el resultado en la matriz C sin usar funciones auxiliares(getvalor y setvalor)
void matmulFilas(double *A, double *B, double *C, int n);
void matmulCol(double *A, double *B, double *C, int n);

//Toma la matriz A ordenada por filas y la ordena por columnas en A_col
void ordenarCol(double *A, double *A_col, int n);

// Valida el resultado. 0 si ok, sino -1
int validar(int n, double *c, char* fileR);
// Lee una matriz desde archivo de a datos consecutivos y la almacena en el mismo orden en memoria principal 
double* leerMatriz(double *m, int n, char * fullpath);
//Para calcular tiempo
double dwalltime(void);

//Retorna el valor de la matriz en la posicion fila y columna segun el orden que este ordenada
double getValor(double *matriz,int fila,int columna,int orden,int N);
//Establece el valor de la matriz en la posicion fila y columna segun el orden que este ordenada
void setValor(double *matriz,int fila,int columna,int orden,double valor,int N);




int main(int argc,char*argv[]){
int N;

	// Chequeo de parametros
	if ( (argc < 2) || ((N = atoi(argv[1])) <= 0)){
		printf("\nError en los parametros. Usar: %s N  <ruta y archivo matriz A> <ruta y archivo matriz B> <ruta y archivo matriz resultado> \n", argv[0]);
		exit(1);
	}

	// Lee las rutas de los archivos
	char* fileA = argv[2];
	//char* fileB = argv[3];
	//char* fileR = argv[4];

	//Aloca memoria para las matrices
	double* A = (double*)malloc(sizeof(double)*N*N);
	double* A_col = (double*)malloc(sizeof(double)*N*N);

	//double* B = (double*)malloc(sizeof(double)*N*N);
	double* C = (double*)malloc(sizeof(double)*N*N);

	// Lee las matrices a y b de archivos. Se almacenan en memoria linealmente tal como se encuentran en archivo
	printf("Leyendo matrices...\n");
	A = leerMatriz(A,N,fileA); // Asumimos ordenada en archivo por filas, en memoria la utilizamos por filas
	A_col = leerMatriz(A_col,N,fileA); // Asumimos ordenada en archivo por filas, en memoria la utilizamos por columnas
	//B = leerMatriz(B,N,fileB); // Asumimos ordenada en archivo por filas, en memoria la utilizamos por filas 

	//Realiza la multiplicacion
	printf("Multiplicando matrices A*A filas...\n");
	double timetick = dwalltime();
		// Realiza la multiplicacion
		matmulFilas	(A, A, C, N);
	double workTime = dwalltime() - timetick;

	printf("mm_naive n = %d Tiempo en segundos %f\n", N, workTime);

	printf("Multiplicando matrices A*A_col ...\n");
	 	double timetick2 = dwalltime();
		// Realiza la multiplicacion
		ordenarCol(A, A_col, N);
		matmulCol(A, A_col, C, N);
		double workTime2 = dwalltime() - timetick2;

	printf("mm_naive n = %d Tiempo en segundos %f\n", N, workTime2);

	// Liberar memoria antes de validar
	free(A);
	free(A_col);
	//free(B);
  
	// Valida
	/*
	
	printf("Validando...\n");
	if (validar(N,C,fileR) == 0)
		printf("Resultado correcto.\n");
	else
		printf("Error.\n");
	*/
  
	// Libera memoria restante
	free(C);
  
 return(0);
}

//-----------------------------------------------------------------

// Dada una matriz A ordenada por filas, la ordena por columnas en A_col
void ordenarCol(double *A, double *A_col, int n){
	int i, j;
	for(i=0;i<n;i++){
		for(j=0;j<n;j++){
			A_col[i+n*j] = A[i*n+j];
		}
	}
}

//Dada una matriz A ordenada por columnas, la ordena por filas en A_filas
void ordenarFilas(double *A, double *A_filas, int n){
	int i, j;
	for(i=0;i<n;i++){
		for(j=0;j<n;j++){
			A_filas[i*n+j] = A[i+n*j];
		}
	}
}

void matmul(double *A, double *B, double *C, int N){
int i, j, k;

	for(i=0;i<N;i++){
		for(j=0;j<N;j++){
			setValor(C,i,j,ORDENXFILAS,0,N);
			for(k=0;k<N;k++){
				setValor(C, i, j, ORDENXFILAS, getValor(C,i,j,ORDENXFILAS,N) + getValor(A,i,k,ORDENXFILAS,N)*getValor(B,k,j,ORDENXFILAS,N), N);
			}
		}
	} 
  
}

//Ejercicio 1b
void matmulFilas(double *A, double *B, double *C, int N){
int i, j, k;

	for(i=0;i<N;i++){
		for(j=0;j<N;j++){

			//Inciso III
			//setValor(C,i,j,ORDENXFILAS,0,N);
			C[i*N+j] = 0;//Acceso por filas
			for(k=0;k<N;k++){

				//inciso III
				//setValor(C, i, j, ORDENXFILAS, getValor(C,i,j,ORDENXFILAS,N) + getValor(A,i,k,ORDENXFILAS,N)*getValor(B,k,j,ORDENXFILAS,N), N);
				C[i*N+j] += A[i*N+k] * B[k*N+j];//Acceso A y B por filas

				//inciso IV
				//C[i*N+j] += A[i*N+k] * B[j*N+k];//Acceso B por columnas

			}
		}
	} 
  
}

/*
HECHO POR NOSOTROS, CASO PARA MATRIZ TRIANGULAR SUPERIOR 
void matmulTriCeros(double *A, double *B, double *C, int N){
int i, j, k;

	//cantidad de ceros matriz triangular de orden N: (N^2-N)/2

	for(i=0;i<N;i++){
		for(j=0;j<N;j++){
			C[i*N+j] = 0;//Acceso por filas
			for(k=0;k<=j;k++){	// Recorro solo hasta j, ya que el resto de los elementos de la fila j son ceros, por lo tanto no aportan nada a la suma
				//if(k%N > k/N){ //Si el elemento es distinto de cero   POCO OPTIMO
					C[i*N+j] += A[i*N+k] * B[j*N+k];//Acceso B por columnas
				//}
			}
		}
	} 
  
}
*/

//CASO GENERAL PARA MATRIZ TRIANGULAR SUPERIOR O INFERIOR, CON CEROS
void matmulTriCeros(double *A, double *B, double *C, int N, int es_inferior) {
    int i, j, k;

    for (i = 0; i < N; i++) {
        for (j = 0; j < N; j++) {
            double suma = 0.0;
            
            // 1. Calculamos los límites antes de multiplicar para no perder tiempo en el bucle
            int k_inicio, k_fin;
            
            if (es_inferior == 1) {
                k_inicio = j;       // Nace en la diagonal
                k_fin = N - 1;      // Va hasta el final de la columna
            } else {
                k_inicio = 0;       // Nace arriba de todo
                k_fin = j;          // Frena en la diagonal
            }
            
            // 2. Ejecutamos el bucle con los límites a medida
            // Nota que usamos "<=" para que incluya el valor de k_fin
            for (k = k_inicio; k <= k_fin; k++) { 
                // Usamos B para representar la matriz triangular (sea L o U)
                suma += A[i*N + k] * B[j*N + k];
            }

            C[i*N + j] = suma;
        }
    } 
}

//CASO GENERAL PARA MATRIZ TRIANGULAR SUPERIOR O INFERIOR, SIN CEROS
void matmulTriSinCeros(double *A, double *B, double *C, int N, int es_inferior) {
    int i, j, k;

    for (i = 0; i < N; i++) {
        for (j = 0; j < N; j++) {
            double suma = 0.0;
            
            if (es_inferior == 1) {
                // CASO 1: Triangular Inferior (L)
                // k nace en j y va hasta el final (N-1)
                for (k = j; k < N; k++) { 
                    // Fórmula de índice para matriz Inferior empaquetada por filas
                    // Matemáticamente estamos en la fila k, columna j
                    int indice = (k * (k + 1)) / 2 + j;
                    
                    suma += A[i*N + k] * B[indice];
                }
            } else {
                // CASO 2: Triangular Superior (U)
                // k nace en 0 y frena en j
                for (k = 0; k <= j; k++) { 
                    // Fórmula de índice para matriz Superior empaquetada por filas
                    // Matemáticamente estamos en la fila k, columna j
                    int indice = (k * N) - (k * (k - 1)) / 2 + (j - k);
                    
                    suma += A[i*N + k] * B[indice];
                }
            }
            
            C[i*N + j] = suma;
        }
    } 
}


void matmulCol(double *A, double *B, double *C, int N){
int i, j, k;

	for(i=0;i<N;i++){
		for(j=0;j<N;j++){

			//Inciso III
			//setValor(C,i,j,ORDENXFILAS,0,N);
			C[i*N+j] = 0;//Acceso por filas
			for(k=0;k<N;k++){

				//inciso III
				//setValor(C, i, j, ORDENXFILAS, getValor(C,i,j,ORDENXFILAS,N) + getValor(A,i,k,ORDENXFILAS,N)*getValor(B,k,j,ORDENXFILAS,N), N);
				//C[i*N+j] += A[i*N+k] * B[k*N+j];//Acceso A y B por filas

				//inciso IV
				C[i*N+j] += A[i*N+k] * B[(j*N)+k];//Acceso B por columnas

			}
		}
	} 
  
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

double getValor(double *matriz,int fila,int columna,int orden,int N){
 if(orden==ORDENXFILAS){
  return(matriz[fila*N+columna]);
 }else{
  return(matriz[fila+columna*N]);
 }
}

void setValor(double *matriz,int fila,int columna,int orden,double valor,int N){
 if(orden==ORDENXFILAS){
  matriz[fila*N+columna]=valor;
 }else{
  matriz[fila+columna*N]=valor;
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
