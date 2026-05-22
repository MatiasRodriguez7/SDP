#ifndef __PTHREADS_FUNCTIONS__
#define __PTHREADS_FUNCTIONS__
typedef struct{
    double * mi_A;
    double * B;
    double * mi_C;
    int T;
    int N;
    int iteraciones;
}par_t;

extern void pthreads_function(par_t);
#endif