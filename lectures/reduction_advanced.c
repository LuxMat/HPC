#include "limits.h"
#include <math.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>


int
main()
{
  const int sz = 1000;
  double *ventries = (double*) malloc(sz*sz*sizeof(double));
  double **v = (double**) malloc(sz*sizeof(double*));
  double *w = (double*) malloc(sz*sizeof(double));
  double sum;

  for ( int ix = 0; ix < sz*sz; ++ix )
    ventries[ix] = ix;
  for ( int ix = 0; ix < sz; ++ix )
    v[ix] = ventries + ix*sz;
  for ( int ix = 0; ix < sz; ++ix )
    w[ix] = 0.0;

  omp_set_num_threads(4);


  printf("arrays\n");

  // we can reduce over an array of variables
  #pragma omp parallel for reduction(+:w[:sz])
  for ( int ix = 0; ix < sz; ++ix )
    for ( int jx = 0; jx < sz; ++jx )
      w[ix] += v[ix][jx];

  printf("%f %f\n", w[0], w[sz-1]);


  printf("\ncollapse\n");

  // the collapse clause instructs OpenMP to split iterations across several
  // levels of iteration. This can drastically increase efficiency.
  sum = 0.;
  #pragma omp parallel for reduction(+:sum)
  for ( int ix = 0; ix < 1000; ++ix )
    for ( int jx = 0; jx < 1000; ++jx )
      sum += ix*jx;

  printf("sum is %lf\n", sum);


  sum = 0.;
  #pragma omp parallel for collapse(2) reduction(+:sum)
  for ( int ix = 0; ix < 8; ++ix )
    for ( int jx = 0; jx < 1000; ++jx )
      sum += ix*jx;

  printf("sum is %lf\n", sum);


  // When collapsing several for loops the product of all lengths must be
  // accommodated by the index type.
  sum = 0.;
  #pragma omp parallel for collapse(2) reduction(+:sum)
  for ( int16_t ix = 0; ix < 1000; ++ix )
    for ( int16_t jx = 0; jx < 1000; ++jx )
      sum += ix*jx;

  printf("sum is %lf\n", sum);


  sum = 0.;
  #pragma omp parallel for reduction(+:sum)
  for ( int16_t ix = 0; ix < 1000; ++ix )
    for ( int16_t jx = 0; jx < 1000; ++jx )
      sum += ix*jx;

  printf("sum is %lf\n", sum);


  printf("\ncustom reduction\n");
  
  // one can implement custom reductions via
  //   #pragma omp declare reduction (identifier : type-list : combiner )
  // or
  //   #pragma omp declare reduction (identifier : type-list : combiner ) \
  //      initializer ( ... )

  // This yields a compiler abort on gcc 10.3.1
  /*

  #pragma omp declare reduction (maxabs : int, long int : abs(omp_in) > omp_out ? abs(omp_in) : omp_out )

  int a = 0;
  #pragma omp parallel for reduction(maxabs:a)
  for ( int ix = 0; ix < sz; ++ix ) {
    int b = ix - sz;
    a = abs(a) > abs(b) ? a : b;
  }
 
  printf("maxabs is %lf\n", a);

  
  #pragma omp declare reduction (minabs : int : \
      abs(omp_in) > omp_out ? omp_out : abs(omp_in) ) \
      initializer ( omp_priv = INT_MAX ) 

  // Since we have defined an initializer, the initialization of a is
  // overwritten.
  a = INT_MAX;
  #pragma omp parallel for reduction(minabs:a)
  for ( int ix = 0; ix < sz; ++ix ) {
    int b = ix - sz;
    a = abs(a) < abs(b) ? a : b;
  }
 
  printf("minabs is %lf\n", a);
  */


  free(v);
  free(w);

  return 0;
}

