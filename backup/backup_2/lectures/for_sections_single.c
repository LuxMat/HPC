#include <math.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

int
main()
{
  const int sz = 1000;
  double *v = (double*) malloc(sz*sizeof(double));
  double *w = (double*) malloc(sz*sizeof(double));
  double sum;

  for ( int ix = 0; ix < sz; ++ix )
    v[ix] = ix+1;

  // The number of threads used by default heavily depends on the
  // circumstances.
  omp_set_num_threads(4);


  printf("parallel for\n");

  // parallel for is the short form for two constructs: parallel and for.
  // Not allowed inside of for construct are break, continue, and return.
  #pragma omp parallel for
  for ( int ix = 0; ix < sz; ++ix )
    w[ix] = 2*v[ix];

  printf("w[0] = %f\n", w[0]);


  #pragma omp parallel
  #pragma omp for
  for ( int ix = 0; ix < sz; ++ix )
    w[ix] = 2*w[ix];

  printf("w[0] = %f\n", w[0]);


  // The parallel construct has scope that can be made explicit via braces.
  #pragma omp parallel
  {
    #pragma omp for
    for ( int ix = 0; ix < sz; ++ix )
      w[ix] = 2*w[ix];
  }

  printf("w[0] = %f\n", w[0]);


  // Without the for construct, this is fatally wrong; no guarantee for what
  // the result will be.
  #pragma omp parallel
  {
    for ( int ix = 0; ix < sz; ++ix )
      w[ix] = 2*w[ix];
  }

  printf("w[0] = %f\n", w[0]);


  printf("\nparallel sections\n");

  // Instead of sharing work of a for loop, one can assign specific sections

  #pragma omp parallel sections
  {
    #pragma omp section
    {
      for ( int ix = 0; ix < sz/2; ++ix )
        w[ix] = 2*w[ix];
      
    }

    #pragma omp section
    {
      for ( int ix = sz/2; ix < sz; ++ix )
        w[ix] = 2*w[ix];
    }
  }
  printf("w[0] = %f\n", w[0]);
  printf("w[sz/2] = %f\n", w[sz/2]);


  printf("\nparallel barriers\n");

  // After every parallel (or single or master) construct, there is an implicit
  // barrier joining all threads in the team. It can be suspended with the
  // nowait clause. Then an (explicit or implicit) barrier has to be invoked
  // before using the results.
  #pragma omp parallel
  {
    #pragma omp for nowait
    for ( int ix = 0; ix < sz/2; ++ix )
      w[ix] = 2*v[ix];

    #pragma omp for
    for ( int ix = sz/2; ix < sz; ++ix )
      w[ix] = 2*v[ix];
  }
  // #pragma omp barrier

  printf("w[0] = %f\n", w[0]);
  printf("w[sz/2] = %f\n", w[sz/2]);


  printf("\nsingle master\n");
  #pragma omp parallel
  { 
    #pragma omp single nowait
    w[0] = 2*v[0];

    #pragma omp single nowait
    w[1] = 2*v[1];

    #pragma omp barrier

    #pragma omp master
    w[2] = w[0] + w[1];
  }
  printf("w[2] = %f\n", w[2]);


  free(v);
  free(w);

  return 0;
}

