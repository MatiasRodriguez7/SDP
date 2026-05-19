#include<stdio.h>
#include<stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <mpi.h>

// Lee una matriz desde archivo de a datos consecutivos y la almacena en el mismo orden en memoria principal 
double* leerMatriz(double *m, int n, char * fullpath);
// Multiplica, por bloques de bsxbs, dos matrices (a y b) de nxn y deja el resultado en la matriz c
void matmulblks(double *mi_a, double *b, double *mi_c, int n, int bs, int iteraciones);
void blkmul(double *ablk, double *bblk, double *cblk, int n, int bs);
// Valida el resultado 0 si ok, sino -1
int validar(int n, double *c, char* fileR);
// Para obtener el tiempo de ejecucion
double dwalltime();


int main(int argc, char *argv[]){
    double *a, *b, *c;
    int n, bs;
    double ini, fin;
    int id, nProcs;

	MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &nProcs);

    // Chequeo de parametros
    if ( (argc < 6) || ((n = atoi(argv[1])) <= 0) || ((bs = atoi(argv[2])) <= 0) || ((n % bs) != 0))
    {
        printf("\nError en los parametros. Usar: %s N BS (N debe ser multiplo de BS) <ruta y archivo matriz A> <ruta y archivo matriz B> <ruta y archivo matriz resultado> \n", argv[0]);
        MPI_Finalize();
        exit(1);
    }

    int iteraciones = n/nProcs;

    char* fileInA = argv[3];
	char* fileInB = argv[4];
	char* fileInR = argv[5];

    double* mi_a = (double *) malloc(iteraciones*n*sizeof(double));
    double* mi_c = (double *) malloc(iteraciones*n*sizeof(double));
    b = (double *) malloc(n*n*sizeof(double));

    if(id==0){
        // Alocar memoria
        a = (double *) malloc(n*n*sizeof(double));
        c = (double *) malloc(n*n*sizeof(double));
        a = leerMatriz(a,n,fileInA); 
        b = leerMatriz(b,n,fileInB); 
        ini = dwalltime();
    }
    

	// 2. El Proceso 0 envía la matriz A_col completa a TODOS los procesos
	MPI_Bcast(b, n*n, MPI_DOUBLE, 0, MPI_COMM_WORLD);

	// 3. El Proceso 0 reparte las filas de A entre todos los procesos
    MPI_Scatter(a, iteraciones * n, MPI_DOUBLE, 
                mi_a, iteraciones * n, MPI_DOUBLE, 
                0, MPI_COMM_WORLD);

    // 4. COMPUTO PARALELO
    matmulblks(mi_a, b, mi_c, n, bs, iteraciones);

	
    // 5. El Proceso 0 recolecta los pedazos calculados (mi_C) y arma la matriz final (C)
    MPI_Gather(mi_c, iteraciones * n, MPI_DOUBLE, c, iteraciones * n, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	
	free(mi_a);
	free(mi_c);
	free(b);

	if(id==0){
		fin = dwalltime() - ini;
		free(a);
        // Valida
        printf("Validando...\n");
        if (validar(n,c,fileInR) == 0)
            printf("Resultado correcto.\n");
        else
            printf("Error.\n");
        // Libera memoria restante
        free(c);
        printf("MMBLK; n:%d ; bs:%d ; Tiempo(seg): %.2lf\n",n,bs,fin);

	}
	MPI_Finalize();
	return(0);
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

void matmulblks(double *mi_a, double *b, double *mi_c, int N, int bs, int iteraciones){
int i, j, k;
int iN, jN, iNj;

    // Inicializa la matriz resultado
	for (i = 0; i < iteraciones*N; i++){
		mi_c[i] = 0;
	} 
    // Multiplica
	for (i = 0; i < iteraciones; i += bs){
		iN = i*N;
		for (j = 0; j < N; j += bs){
			jN = j*N;
			iNj = iN + j;
			for  (k = 0; k < N; k += bs){
				blkmul(&mi_a[iN + k], &b[jN + k], &mi_c[iNj], N, bs);
			}
			
		}
	}

}

void blkmul(double *ablk, double *bblk, double *cblk, int N, int bs){
 int i, j, k;
 int iN, jN, iNj;

  for (i = 0; i < bs; i++){
    iN = i*N;
    for (j = 0; j < bs; j++){
	  jN = j*N;	
	  iNj = iN + j;
      for  (k = 0; k < bs; k++){  
        cblk[iNj] += ablk[iN + k] * bblk[jN + k];
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

/*****************************************************************/

#include <stdio.h>
#include <sys/time.h>

double dwalltime()
{
	double sec;
	struct timeval tv;

	gettimeofday(&tv,NULL);
	sec = tv.tv_sec + tv.tv_usec/1000000.0;
	return sec;
}



